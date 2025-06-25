// #my_engine_source_file


#pragma once

#define MY_PLATFORM_BREAK __asm__ volatile("int $0x03")
#define MY_PLATFORM_ABORT (std::abort())
