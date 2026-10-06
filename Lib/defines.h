///////////////////////////////////////////////////////////////////////////////
// Global defines
///////////////////////////////////////////////////////////////////////////////
#ifndef V3DLIB_DEFINES_H
#define V3DLIB_DEFINES_H

#define OUTPUT_COMPILEDATA

//
// Enable following to allow v3d instructions (Source and Target)
// for the interpreter and emulator.
//
// At time of writing (20261004), this will almost certainly fail on execution.
// Added to update interpreter/emulator to v3d. Shouldn't be enabled on public commit.
//
// Despite name, it also applies to emulator.
//
//#define V3D_ALLOW_INTERPRET

// Show ugly nag message, to prevent commit to github with this enabled
#ifdef V3D_ALLOW_INTERPRET
#pragma message("WARNING: V3D_ALLOW_INTERPRET enabled")
#endif

#if __GNUC__
#else
#pragma message("WARNING: Using compiler other than GCC. This is not supported (you're on your own)")
#endif


// GCC directives for ARM compilation

#ifdef __aarch64__
  #define ARM64
#else
  #ifdef __arm__    // apparently not set for ARM 64 bits
    #define ARM32
  #endif
#endif

#endif  // V3DLIB_DEFINES_H
