/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "EASTLBenchmark.h"
#include "EASTLTest.h"
#include "EAStopwatch.h"
#include <algorithm>
#include <string>
#include <safememory/string.h>
#include <EASTL/string.h>

EA_DISABLE_ALL_VC_WARNINGS()
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
EA_RESTORE_ALL_VC_WARNINGS()

// namespace safememory {
// 	template<class T>
// 	using basic_string = std::basic_string<T>;

// 	using string = std::string;
// 	using wstring = std::wstring;
// 	using u16string = std::u16string;
// 	using u32string = std::u32string;
// }

using namespace EA;
using EA::StdC::Stopwatch;

namespace
{
	template <typename Container> 
	void TestPushBack(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		stopwatch.Restart();
		for(int i = 0; i < 100000; i++)
			c.push_back((typename Container::value_type)(i & ((typename Container::value_type)~0)));
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestInsert1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p)
	{
		const typename Container::size_type s = c.size();
		stopwatch.Restart();
		for(int i = 0; i < 100; i++)
			c.insert(s - (typename Container::size_type)(i * 317), p);
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestErase1(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		const typename Container::size_type s = c.size();
		stopwatch.Restart();
		for(int i = 0; i < 100; i++)
			c.erase(s - (typename Container::size_type)(i * 339), 7);
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestReplace1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p)
	{
		int n = p.size();
		const typename Container::size_type s = c.size();
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			c.replace(s - (typename Container::size_type)(i * 5), ((n - 2) + (i & 3)), p); // The second argument rotates through n-2, n-1, n, n+1, n-2, etc.
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestReserve(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		const typename Container::size_type s = c.capacity();
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			c.reserve((s - 2) + (i & 3)); // The second argument rotates through n-2, n-1, n, n+1, n-2, etc.
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestSize(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.size());
		stopwatch.Stop();
	}


	template <typename Container>
	void TestBracket(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		int32_t temp = 0;
		stopwatch.Restart();
		for(typename Container::size_type j = 0, jEnd = c.size(); j < jEnd; j++)
			temp += c[j];
		stopwatch.Stop();
		sprintf(Benchmark::gScratchBuffer, "%u", (unsigned)temp);
	}


	template <typename Container>
	void TestFind(EA::StdC::Stopwatch& stopwatch, Container& c)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, *std::find(c.begin(), c.end(), (typename Container::value_type)~0));
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestFind1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.find(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestRfind1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.rfind(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}

	template <typename Container> 
	void TestFirstOf1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.find_first_of(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}

	template <typename Container> 
	void TestLastOf1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.find_last_of(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}

	template <typename Container> 
	void TestFirstNotOf1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.find_first_not_of(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}

	template <typename Container> 
	void TestLastNotOf1(EA::StdC::Stopwatch& stopwatch, Container& c, Container& p, int pos)
	{
		stopwatch.Restart();
		for(int i = 0; i < 1000; i++)
			Benchmark::DoNothing(&c, c.find_last_not_of(p, (typename Container::size_type)pos));
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestCompare(EA::StdC::Stopwatch& stopwatch, Container& c1, Container& c2) // size()
	{
		stopwatch.Restart();
		for(int i = 0; i < 500; i++)
			Benchmark::DoNothing(&c1, c1.compare(c2));
		stopwatch.Stop();
	}


	template <typename Container> 
	void TestSwap(EA::StdC::Stopwatch& stopwatch, Container& c1, Container& c2) // size()
	{
		stopwatch.Restart();
		for(int i = 0; i < 10000; i++) // Make sure this is an even count so that when done things haven't changed.
		{
			c1.swap(c2);
			Benchmark::DoNothing(&c1);
		} 
		stopwatch.Stop();
	}

} // namespace

template<int IX, class S8, class S16>
void BenchmarkStringTempl()
{
	Stopwatch stopwatch1(Stopwatch::kUnitsCPUCycles);

	for(int i = 0; i < 2; i++)
	{
		S8  		stds8(16, 0);
		S16   		stds16(16, 0);

		///////////////////////////////
		// Test push_back
		///////////////////////////////

		TestPushBack(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/push_back", IX, stopwatch1);

		TestPushBack(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/push_back", IX, stopwatch1);


		///////////////////////////////
		// Test insert(size_type position, const value_type* p)
		///////////////////////////////

		decltype(stds8) pInsert1_8 = { 'a', 0 };
		TestInsert1(stopwatch1, stds8, pInsert1_8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/insert/pos,p", IX, stopwatch1);

		decltype(stds16) pInsert1_16 = { 'a', 0 };
		TestInsert1(stopwatch1, stds16, pInsert1_16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/insert/pos,p", IX, stopwatch1);


		///////////////////////////////
		// Test erase(size_type position, size_type n)
		///////////////////////////////

		TestErase1(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/erase/pos,n", IX, stopwatch1);

		TestErase1(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/erase/pos,n", IX, stopwatch1);


		///////////////////////////////            
		// Test replace(size_type position, size_type n1, const basic_string& str)
		///////////////////////////////

		decltype(stds8) pReplace1_stds8 = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };

		TestReplace1(stopwatch1, stds8, pReplace1_stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/replace/pos,n,str", IX, stopwatch1);

		decltype(stds16) pReplace1_stds16 = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };

		TestReplace1(stopwatch1, stds16, pReplace1_stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/replace/pos,n,str", IX, stopwatch1);


		///////////////////////////////
		// Test reserve(size_type)
		///////////////////////////////

		TestReserve(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/reserve", IX, stopwatch1);

		TestReserve(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/reserve", IX, stopwatch1);


		///////////////////////////////
		// Test size()
		///////////////////////////////

		TestSize(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/size", IX, stopwatch1);

		TestSize(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/size", IX, stopwatch1);


		///////////////////////////////
		// Test operator[].
		///////////////////////////////

		TestBracket(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/operator[]", IX, stopwatch1);

		TestBracket(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/operator[]", IX, stopwatch1);


		///////////////////////////////
		// Test iteration via find().
		///////////////////////////////

		TestFind(stopwatch1, stds8);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/iteration", IX, stopwatch1);

		TestFind(stopwatch1, stds16);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/iteration", IX, stopwatch1);


		///////////////////////////////
		// Test find(const basic_string& str, size_type position)
		///////////////////////////////

		decltype(stds8) pFind1_stds8 = { 'p', 'a', 't', 't', 'e', 'r', 'n' };

		stds8.insert(stds8.size() / 2, pFind1_stds8);

		TestFind1(stopwatch1, stds8, pFind1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/find/str,pos", IX, stopwatch1);

		decltype(stds16) pFind1_stds16 = { 'p', 'a', 't', 't', 'e', 'r', 'n' };

		stds16.insert(stds16.size() / 2, pFind1_stds16);

		TestFind1(stopwatch1, stds16, pFind1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/find/str,pos", IX, stopwatch1);


		///////////////////////////////
		// Test rfind(const basic_string& str, size_type position)
		///////////////////////////////

		TestRfind1(stopwatch1, stds8, pFind1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/rfind/str,pos", IX, stopwatch1);

		TestRfind1(stopwatch1, stds16, pFind1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/rfind/str,pos", IX, stopwatch1);


		///////////////////////////////
		// Test find_first_of(const basic_string& str, size_type position)
		///////////////////////////////

		const int kFindOf1Size = 7;
		decltype(stds8) pFindOf1_stds8 = { '~', '~', '~', '~', '~', '~', '~' };

		TestFirstOf1(stopwatch1, stds8, pFindOf1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/find_first_of/str,pos", IX, stopwatch1);

		decltype(stds16) pFindOf1_stds16 = { '~', '~', '~', '~', '~', '~', '~' };

		TestFirstOf1(stopwatch1, stds16, pFindOf1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/find_first_of/str,pos", IX, stopwatch1);


		///////////////////////////////
		// Test find_last_of(const basic_string& str, size_type position)
		///////////////////////////////

		TestLastOf1(stopwatch1, stds8, pFindOf1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/find_last_of/str,pos", IX, stopwatch1);

		TestLastOf1(stopwatch1, stds16, pFindOf1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/find_last_of/str,pos", IX, stopwatch1);


		///////////////////////////////
		// Test find_first_not_of(const basic_string& str, size_type position)
		///////////////////////////////

		TestFirstNotOf1(stopwatch1, stds8, pFind1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/find_first_not_of/str,pos", IX, stopwatch1);

		TestFirstNotOf1(stopwatch1, stds16, pFind1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/find_first_not_of/str,pos", IX, stopwatch1);


		///////////////////////////////
		// Test find_last_of(const basic_string& str, size_type position)
		///////////////////////////////

		TestLastNotOf1(stopwatch1, stds8, pFind1_stds8, 15);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/find_last_of/str,pos", IX, stopwatch1);

		TestLastNotOf1(stopwatch1, stds16, pFind1_stds16, 15);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/find_last_of/str,pos", IX, stopwatch1);

		///////////////////////////////
		// Test compare()
		///////////////////////////////

		decltype(stds8)  stds8X(stds8);

		TestCompare(stopwatch1, stds8, stds8X);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/compare", IX, stopwatch1);

		decltype(stds16) stds16X(stds16);

		TestCompare(stopwatch1, stds16, stds16X);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/compare", IX, stopwatch1);



		///////////////////////////////
		// Test swap()
		///////////////////////////////

		TestSwap(stopwatch1, stds8, stds8X);

		if(i == 1)
			Benchmark::AddResult("string<char8_t>/swap", IX, stopwatch1);

		TestSwap(stopwatch1, stds16, stds16X);

		if(i == 1)
			Benchmark::AddResult("string<char16_t>/swap", IX, stopwatch1);

	}
}


void BenchmarkString()
{
	EASTLTest_Printf("String\n");

	// typedef std::basic_string<char8_t> Std8;
	// typedef	std::basic_string<char16_t> Std16;

	typedef eastl::basic_string<char8_t> Ea8;
	typedef eastl::basic_string<char16_t> Ea16;

	typedef safememory::basic_string<char8_t, safememory::memory_safety::none> Unsafe8;
	typedef safememory::basic_string<char16_t, safememory::memory_safety::none> Unsafe16;

	typedef safememory::basic_string<char8_t, safememory::memory_safety::safe> Safe8;
	typedef safememory::basic_string<char16_t, safememory::memory_safety::safe> Safe16;

	typedef safememory::basic_string_safe<char8_t, safememory::memory_safety::safe> VerySafe8;
	typedef safememory::basic_string_safe<char16_t, safememory::memory_safety::safe> VerySafe16;

	// BenchmarkStringTempl<1, Std8, Std16>();
	BenchmarkStringTempl<1, Ea8, Ea16>();
	BenchmarkStringTempl<2, Unsafe8, Unsafe16>();
	BenchmarkStringTempl<3, Safe8, Safe16>();
	BenchmarkStringTempl<4, VerySafe8, VerySafe16>();
}









