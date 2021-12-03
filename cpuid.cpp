// InstructionSet.cpp 
// Compile by using: cl /EHsc /W4 InstructionSet.cpp
// processor: x86, x64
// Uses the __cpuid intrinsic to get information about 
// CPU extended instruction set support.

#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>
#include "cpuid.h"

#include "util/util.h"

class InstructionSet
{
	// forward declarations
	class InstructionSet_Internal;

public:
	// getters
	static std::string Vendor(void) { return CPU_Rep.vendor_; }
	static std::string Brand(void) { return CPU_Rep.brand_; }

	static bool SSE(void) { return CPU_Rep.f_1_EDX_[25]; }
	static bool SSE2(void) { return CPU_Rep.f_1_EDX_[26]; }
	static bool SSE3(void) { return CPU_Rep.f_1_ECX_[0]; }
	static bool SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }
	static bool SSE41(void) { return CPU_Rep.f_1_ECX_[19]; }
	static bool SSE42(void) { return CPU_Rep.f_1_ECX_[20]; }

	static bool POPCNT(void) { return CPU_Rep.f_1_ECX_[23]; }

	static bool AVX(void) { return CPU_Rep.f_1_ECX_[28]; }
	static bool AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }
	static bool AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }
	static bool AVX512PF(void) { return CPU_Rep.f_7_EBX_[26]; }
	static bool AVX512ER(void) { return CPU_Rep.f_7_EBX_[27]; }
	static bool AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }

	static bool BMI1(void) { return CPU_Rep.f_7_EBX_[3]; }
	static bool BMI2(void) { return CPU_Rep.f_7_EBX_[8]; }

private:
	static const InstructionSet_Internal CPU_Rep;

	class InstructionSet_Internal
	{
	public:
		InstructionSet_Internal()
			: nIds_{ 0 },
			nExIds_{ 0 },
			isIntel_{ false },
			isAMD_{ false },
			f_1_ECX_{ 0 },
			f_1_EDX_{ 0 },
			f_7_EBX_{ 0 },
			f_7_ECX_{ 0 },
			f_81_ECX_{ 0 },
			f_81_EDX_{ 0 },
			data_{},
			extdata_{}
		{
			//int cpuInfo[4] = {-1};
			std::array<int, 4> cpui;

			// Calling __cpuid with 0x0 as the function_id argument
			// gets the number of the highest valid function ID.
			__cpuid(cpui.data(), 0);
			nIds_ = cpui[0];

			for (int i = 0; i <= nIds_; ++i)
			{
				__cpuidex(cpui.data(), i, 0);
				data_.push_back(cpui);
			}

			// Capture vendor string
			char vendor[0x20];
			memset(vendor, 0, sizeof(vendor));
			*reinterpret_cast<int*>(vendor) = data_[0][1];
			*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
			*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
			vendor_ = vendor;
			if (vendor_ == "GenuineIntel")
			{
				isIntel_ = true;
			}
			else if (vendor_ == "AuthenticAMD")
			{
				isAMD_ = true;
			}

			// load bitset with flags for function 0x00000001
			if (nIds_ >= 1)
			{
				f_1_ECX_ = data_[1][2];
				f_1_EDX_ = data_[1][3];
			}

			// load bitset with flags for function 0x00000007
			if (nIds_ >= 7)
			{
				f_7_EBX_ = data_[7][1];
				f_7_ECX_ = data_[7][2];
			}

			// Calling __cpuid with 0x80000000 as the function_id argument
			// gets the number of the highest valid extended ID.
			__cpuid(cpui.data(), 0x80000000);
			nExIds_ = cpui[0];

			char brand[0x40];
			memset(brand, 0, sizeof(brand));

			for (int i = 0x80000000; i <= nExIds_; ++i)
			{
				__cpuidex(cpui.data(), i, 0);
				extdata_.push_back(cpui);
			}

			// load bitset with flags for function 0x80000001
			if (nExIds_ >= 0x80000001)
			{
				f_81_ECX_ = extdata_[1][2];
				f_81_EDX_ = extdata_[1][3];
			}

			// Interpret CPU brand string if reported
			if (nExIds_ >= 0x80000004)
			{
				memcpy(brand, extdata_[2].data(), sizeof(cpui));
				memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
				memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
				brand_ = brand;
			}
		};

		int nIds_;
		int nExIds_;
		std::string vendor_;
		std::string brand_;
		bool isIntel_;
		bool isAMD_;
		std::bitset<32> f_1_ECX_;
		std::bitset<32> f_1_EDX_;
		std::bitset<32> f_7_EBX_;
		std::bitset<32> f_7_ECX_;
		std::bitset<32> f_81_ECX_;
		std::bitset<32> f_81_EDX_;
		std::vector<std::array<int, 4>> data_;
		std::vector<std::array<int, 4>> extdata_;
	};
};

// Initialize static member data
const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;


// Print out supported instruction set extensions
int cpuid()
{
	auto& outstream = std::cout;

	std::cout << InstructionSet::Vendor() << std::endl;
	std::cout << InstructionSet::Brand() << std::endl;

	// display logical cores	
	acout() << util::core_info();

	auto support_message = [&outstream](std::string isa_feature, bool is_supported)
	{
		if (is_supported)
			outstream << isa_feature << " supported" << std::endl;
	};
	support_message("SSE", InstructionSet::SSE());
	support_message("SSE2", InstructionSet::SSE2());
	support_message("SSE3", InstructionSet::SSE3());
	support_message("SSE4.1", InstructionSet::SSE41());
	support_message("SSE4.2", InstructionSet::SSE42());
	support_message("SSE4a", InstructionSet::SSE4a());

	support_message("POPCNT", InstructionSet::POPCNT());

	support_message("AVX", InstructionSet::AVX());
	support_message("AVX2", InstructionSet::AVX2());
	support_message("AVX512CD", InstructionSet::AVX512CD());
	support_message("AVX512ER", InstructionSet::AVX512ER());
	support_message("AVX512F", InstructionSet::AVX512F());
	support_message("AVX512PF", InstructionSet::AVX512PF());

	support_message("BMI1", InstructionSet::BMI1());
	support_message("BMI2", InstructionSet::BMI2());

	return 0;
}