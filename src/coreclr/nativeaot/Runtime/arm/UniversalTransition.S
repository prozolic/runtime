// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

.syntax unified
.thumb

#include <AsmOffsets.inc>         // generated by the build from AsmOffsets.cpp
#include <unixasmmacros.inc>

#ifdef FEATURE_DYNAMIC_CODE

#ifdef _DEBUG
#define TRASH_SAVED_ARGUMENT_REGISTERS
#endif

#define COUNT_ARG_REGISTERS (4)
#define INTEGER_REGISTER_SIZE (4)
#define ARGUMENT_REGISTERS_SIZE (COUNT_ARG_REGISTERS * INTEGER_REGISTER_SIZE)

// Largest return block is 4 doubles
#define RETURN_BLOCK_SIZE (32)

#define COUNT_FLOAT_ARG_REGISTERS (8)
#define FLOAT_REGISTER_SIZE (8)
#define FLOAT_ARG_REGISTERS_SIZE (COUNT_FLOAT_ARG_REGISTERS * FLOAT_REGISTER_SIZE)

#define PUSHED_LR_SIZE (4)
#define PUSHED_R11_SIZE (4)

//
// From CallerSP to ChildSP, the stack frame is composed of the following adjacent regions:
//
//      ARGUMENT_REGISTERS_SIZE
//      RETURN_BLOCK_SIZE
//      FLOAT_ARG_REGISTERS_SIZE
//      PUSHED_LR
//      PUSHED_R11


#define DISTANCE_FROM_CHILDSP_TO_RETURN_BLOCK (PUSHED_R11_SIZE + PUSHED_LR_SIZE + FLOAT_ARG_REGISTERS_SIZE)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RhpUniversalTransition
//
// At input to this function, r0-3, d0-7 and the stack may contain any number of arguments.
//
// In addition, there are 2 extra arguments passed in the RED ZONE (8 byte negative space
// off of sp).
// sp-4 will contain the managed function that is to be called by this transition function
// sp-8 will contain the pointer sized extra argument to the managed function
//
// When invoking the callee:
//
//  r0 shall contain a pointer to the TransitionBlock
//  r1 shall contain the value that was in sp-8 at entry to this function
//
// Frame layout is:
//
//  {StackPassedArgs}                           ChildSP+078     CallerSP+000
//  {IntArgRegs (r0-r3) (0x10 bytes)}           ChildSP+068     CallerSP-010
//  {ReturnBlock (0x20 bytes)}                  ChildSP+048     CallerSP-030
//   -- The base address of the Return block is the TransitionBlock pointer, the floating point args are
//      in the neg space of the TransitionBlock pointer.  Note that the callee has knowledge of the exact
//      layout of all pieces of the frame that lie at or above the pushed floating point registers.
//  {FpArgRegs (d0-d7) (0x40 bytes)}            ChildSP+008     CallerSP-070
//  {PushedLR}                                  ChildSP+004     CallerSP-074
//  {PushedR11}                                 ChildSP+000     CallerSP-078
//
// NOTE: If the frame layout ever changes, the C++ UniversalTransitionStackFrame structure
// must be updated as well.
//
// NOTE: The callee receives a pointer to the base of the ReturnBlock, and the callee has
// knowledge of the exact layout of all pieces of the frame that lie at or above the pushed
// FpArgRegs.
//
// NOTE: The stack walker guarantees that conservative GC reporting will be applied to
// everything between the base of the ReturnBlock and the top of the StackPassedArgs.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

.macro UNIVERSAL_TRANSITION FunctionName

NESTED_ENTRY Rhp\FunctionName, _TEXT, NoHandler
        // Save argument registers (including floating point) and the return address.
        // NOTE: While we do that, capture the two arguments in the red zone into r12 and r3.
        ldr          r12, [sp, #-4]          // Capture first argument from red zone into r12
        PROLOG_PUSH  "{r3}"                  // Push r3
        ldr          r3, [sp, #-4]           // Capture second argument from red zone into r3
        PROLOG_PUSH  "{r0-r2}"               // Push the rest of the registers
        PROLOG_STACK_ALLOC RETURN_BLOCK_SIZE // Save space a buffer to be used to hold return buffer data.
        PROLOG_VPUSH {d0-d7}                 // Capture the floating point argument registers
        PROLOG_PUSH  "{r11,lr}"              // Save caller's frame chain pointer and PC

        // Setup the arguments to the transition thunk.
        mov         r1, r3

#ifdef TRASH_SAVED_ARGUMENT_REGISTERS

        // Before calling out, trash all of the argument registers except the ones (r0, r1) that
        // hold outgoing arguments.  All of these registers have been saved to the transition
        // frame, and the code at the call target is required to use only the transition frame
        // copies when dispatching this call to the eventual callee.

        PREPARE_EXTERNAL_VAR RhpFpTrashValues, r3
        vldr        d0, [r3, #(0 * 8)]
        vldr        d1, [r3, #(1 * 8)]
        vldr        d2, [r3, #(2 * 8)]
        vldr        d3, [r3, #(3 * 8)]
        vldr        d4, [r3, #(4 * 8)]
        vldr        d5, [r3, #(5 * 8)]
        vldr        d6, [r3, #(6 * 8)]
        vldr        d7, [r3, #(7 * 8)]

        PREPARE_EXTERNAL_VAR RhpIntegerTrashValues, r3
        ldr         r2, [r3, #(2 * 4)]
        ldr         r3, [r3, #(3 * 4)]

#endif // TRASH_SAVED_ARGUMENT_REGISTERS

        // Make the ReturnFromUniversalTransition alternate entry 4 byte aligned
        .balign 4
        add         r0, sp, #DISTANCE_FROM_CHILDSP_TO_RETURN_BLOCK  // First parameter to target function is a pointer to the return block
        blx         r12

GLOBAL_LABEL ReturnFrom\FunctionName

        // Move the result (the target address) to r12 so it doesn't get overridden when we restore the
        // argument registers. Additionally make sure the thumb2 bit is set.
        orr	         r12, r0, #1

        // Restore caller's frame chain pointer and PC.
        EPILOG_POP   "{r11,lr}"

        // Restore the argument registers.
        EPILOG_VPOP  {d0-d7}
        EPILOG_STACK_FREE RETURN_BLOCK_SIZE        // pop return block conservatively reported area
        EPILOG_POP   "{r0-r3}"

        // Tailcall to the target address.
        EPILOG_BRANCH_REG r12

NESTED_END Rhp\FunctionName, _TEXT

.endm

// To enable proper step-in behavior in the debugger, we need to have two instances
// of the thunk. For the first one, the debugger steps into the call in the function,
// for the other, it steps over it.
UNIVERSAL_TRANSITION UniversalTransition
UNIVERSAL_TRANSITION UniversalTransition_DebugStepTailCall

#endif // FEATURE_DYNAMIC_CODE
