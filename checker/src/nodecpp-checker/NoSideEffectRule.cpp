/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/


#include "NoSideEffectRule.h"
#include "FlagRiia.h"
#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {


class NoSideEffectASTVisitor
  : public RecursiveASTVisitor<NoSideEffectASTVisitor> {

  typedef RecursiveASTVisitor<NoSideEffectASTVisitor> Super;

  ClangTidyContext *Context;
  FunctionDecl *CurrentFunc = nullptr;

  /// \brief flags if we are currently visiting a \c [[no_side_effect]] function or method 
  bool NoSideEffect = false;

  /// \brief flags if we are currently visiting a \c [[check_as_user_code]] namespace 
  bool CheckAsUserCode = false;


  std::string
  getTemplateArgumentBindingsText(const TemplateParameterList *Params,
                                        const TemplateArgumentList &Args) {
    return getTemplateArgumentBindingsText(Params, Args.data(), Args.size());
  }

  std::string
  getTemplateArgumentBindingsText(const TemplateParameterList *Params,
                                        const TemplateArgument *Args,
                                        unsigned NumArgs) {
    SmallString<128> Str;
    llvm::raw_svector_ostream Out(Str);

    if (!Params || Params->size() == 0 || NumArgs == 0)
      return std::string();

    for (unsigned I = 0, N = Params->size(); I != N; ++I) {
      if (I >= NumArgs)
        break;

      if (I == 0)
        Out << "[with ";
      else
        Out << ", ";

      if (const IdentifierInfo *Id = Params->getParam(I)->getIdentifier()) {
        Out << Id->getName();
      } else {
        Out << '$' << I;
      }

      Out << " = ";
      Args[I].print(Context->getASTContext()->getPrintingPolicy(), Out);
    }

    Out << ']';
    return Out.str();
  }

  /// \brief Add a diagnostic with the check's name.
  void diag(SourceLocation Loc, StringRef Message) {
    Context->diagError2(Loc, "no-side-effect", Message);
    if(CurrentFunc) {
      if(CurrentFunc->isTemplateInstantiation()) {
        // if(auto Pt = CurrentFunc->getPrimaryTemplate()) {
        //   auto ArgL = CurrentFunc->getTemplateSpecializationArgs();
        //   std::string Text = getTemplateArgumentBindingsText(
        //     Pt->getTemplateParameters(), *ArgL);
          
          Context->diagNote(CurrentFunc->getPointOfInstantiation(), "Instantiated here");
        // }
      }
    }
  }
  
  CheckHelper* getCheckHelper() const { return Context->getCheckHelper(); }

public:

  bool shouldVisitImplicitCode() const { return true; }
  bool shouldVisitTemplateInstantiations() const { return true; }  

  explicit NoSideEffectASTVisitor(ClangTidyContext *Context): Context(Context) {}


  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    if(!D)
      return true;
    //TranslationUnitDecl has an invalid location, but needs traversing anyway
    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);
    else if (auto Ns = dyn_cast<NamespaceDecl>(D)) {
      if(Ns->hasAttr<SafeMemoryCheckAtInstantiationAttr>()) {
        FlagRiia R(CheckAsUserCode);
        return Super::TraverseDecl(D);
      }
      else
        return Super::TraverseDecl(D);
    }
    else if(CheckAsUserCode)
      return Super::TraverseDecl(D);
    else if(isSystemLocation(Context, D->getLocation()))
      return true;
    else      
      return Super::TraverseDecl(D);
  }

  bool TraverseFunctionDecl(clang::FunctionDecl *D) {

    CurrentFunc = D;
    if(NoSideEffect) {
      diag(D->getLocation(), "internal error");
      return false;
    }

    // if(isSystemSafeFunction(D, Context)) {
    //   // don't traverse, assume is ok
    //   return true;
    // }

    bool Flag = getCheckHelper()->isNoSideEffect(D);

    bool Result = true;
    if(D->doesThisDeclarationHaveABody()) {
      NoSideEffect = Flag;
      Result = TraverseStmt(D->getBody()); // Function body.
      NoSideEffect = false;
    }
    return Result;
  }

  bool TraverseCXXMethodDecl(clang::CXXMethodDecl *D) {

    return TraverseFunctionDecl(D);
  }

  bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl *D) {

    return TraverseFunctionDecl(D);
  }

  bool VisitCallExpr(CallExpr *E) {

    if(NoSideEffect && !E->isTypeDependent()) {

      if(!getCheckHelper()->isNoSideEffect(E->getDirectCallee())) {
        diag(E->getExprLoc(), "function with no_side_effect attribute can call only other no side effect functions");
      }
    }

    return Super::VisitCallExpr(E);
  }

  bool VisitLambdaExpr(LambdaExpr *E) {

    if(NoSideEffect && !E->isTypeDependent()) {
      diag(E->getExprLoc(), "lambda not supported inside no_side_effect function");
      return true;
    }

    return Super::VisitLambdaExpr(E);
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *E) {

    if(NoSideEffect && !E->isTypeDependent()) {
      if(!E->getConstructor()->isTrivial()) {
        diag(E->getExprLoc(), "function with no_side_effect attribute can call only other no side effect functions");
      }
    }

    return Super::VisitCXXConstructExpr(E);
  }
};


class NoSideEffectASTConsumer : public ASTConsumer {

  NoSideEffectASTVisitor Visitor;

public:
  NoSideEffectASTConsumer(ClangTidyContext *Context) :Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

std::unique_ptr<clang::ASTConsumer> makeNoSideEffectRule(ClangTidyContext *Context) {
  return llvm::make_unique<NoSideEffectASTConsumer>(Context);
}

} // namespace checker
} // namespace nodecpp


