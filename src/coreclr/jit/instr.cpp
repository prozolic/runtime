// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Instruction                                     XX
XX                                                                           XX
XX          The interface to generate a machine-instruction.                 XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#ifdef _MSC_VER
#pragma hdrstop
#endif

#include "codegen.h"
#include "instr.h"
#include "emit.h"

/*****************************************************************************/

//-----------------------------------------------------------------------------
// genInsName: Returns the string representation of the given CPU instruction, as
// it exists in the instruction table. Note that some architectures don't encode the
// name completely in the table: xarch sometimes prepends a "v", and arm sometimes
// appends a "s". Use `genInsDisplayName()` to get a fully-formed name.
//
const char* CodeGen::genInsName(instruction ins)
{
    // clang-format off
    static
    const char * const insNames[] =
    {
#if defined(TARGET_XARCH)
        #define INST0(id, nm, um, mr,                 lat, tp, tt, flags) nm,
        #define INST1(id, nm, um, mr,                 lat, tp, tt, flags) nm,
        #define INST2(id, nm, um, mr, mi,             lat, tp, tt, flags) nm,
        #define INST3(id, nm, um, mr, mi, rm,         lat, tp, tt, flags) nm,
        #define INST4(id, nm, um, mr, mi, rm, a4,     lat, tp, tt, flags) nm,
        #define INST5(id, nm, um, mr, mi, rm, a4, rr, lat, tp, tt, flags) nm,
        #include "instrs.h"

#elif defined(TARGET_ARM)
        #define INST1(id, nm, fp, ldst, fmt, e1                                 ) nm,
        #define INST2(id, nm, fp, ldst, fmt, e1, e2                             ) nm,
        #define INST3(id, nm, fp, ldst, fmt, e1, e2, e3                         ) nm,
        #define INST4(id, nm, fp, ldst, fmt, e1, e2, e3, e4                     ) nm,
        #define INST5(id, nm, fp, ldst, fmt, e1, e2, e3, e4, e5                 ) nm,
        #define INST6(id, nm, fp, ldst, fmt, e1, e2, e3, e4, e5, e6             ) nm,
        #define INST8(id, nm, fp, ldst, fmt, e1, e2, e3, e4, e5, e6, e7, e8     ) nm,
        #define INST9(id, nm, fp, ldst, fmt, e1, e2, e3, e4, e5, e6, e7, e8, e9 ) nm,
        #include "instrs.h"

#elif defined(TARGET_ARM64)
        #define INST1(id, nm, ldst, fmt, e1                                 ) nm,
        #define INST2(id, nm, ldst, fmt, e1, e2                             ) nm,
        #define INST3(id, nm, ldst, fmt, e1, e2, e3                         ) nm,
        #define INST4(id, nm, ldst, fmt, e1, e2, e3, e4                     ) nm,
        #define INST5(id, nm, ldst, fmt, e1, e2, e3, e4, e5                 ) nm,
        #define INST6(id, nm, ldst, fmt, e1, e2, e3, e4, e5, e6             ) nm,
        #define INST9(id, nm, ldst, fmt, e1, e2, e3, e4, e5, e6, e7, e8, e9 ) nm,
        #include "instrs.h"

        #define INST1(id, nm, info, fmt, e1                                                     ) nm,
        #define INST2(id, nm, info, fmt, e1, e2                                                 ) nm,
        #define INST3(id, nm, info, fmt, e1, e2, e3                                             ) nm,
        #define INST4(id, nm, info, fmt, e1, e2, e3, e4                                         ) nm,
        #define INST5(id, nm, info, fmt, e1, e2, e3, e4, e5                                     ) nm,
        #define INST6(id, nm, info, fmt, e1, e2, e3, e4, e5, e6                                 ) nm,
        #define INST7(id, nm, info, fmt, e1, e2, e3, e4, e5, e6, e7                             ) nm,
        #define INST8(id, nm, info, fmt, e1, e2, e3, e4, e5, e6, e7, e8                         ) nm,
        #define INST9(id, nm, info, fmt, e1, e2, e3, e4, e5, e6, e7, e8, e9                     ) nm,
        #define INST11(id, nm, info, fmt, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10,e11           ) nm,
        #define INST13(id, nm, info, fmt, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13) nm,
        #include "instrsarm64sve.h"

#elif defined(TARGET_LOONGARCH64)
        #define INST(id, nm, ldst, e1, msk, fmt) nm,
        #include "instrs.h"

#elif defined(TARGET_RISCV64)
        #define INST(id, nm, ldst, e1) nm,
        #include "instrs.h"

#else
#error "Unknown TARGET"
#endif
    };
    // clang-format on

    assert((unsigned)ins < ArrLen(insNames));
    assert(insNames[ins] != nullptr);

    return insNames[ins];
}

//-----------------------------------------------------------------------------
// genInsDisplayName: Get a fully-formed instruction display name. This only handles
// the xarch case of prepending a "v", not the arm case of appending an "s".
// This can be called up to four times in a single 'printf' before the static buffers
// get reused.
//
// Returns:
//    String with instruction name
//
const char* CodeGen::genInsDisplayName(emitter::instrDesc* id)
{
    instruction ins     = id->idIns();
    const char* insName = genInsName(ins);

#ifdef TARGET_XARCH
    const emitter* emit      = GetEmitter();
    const char*    vexPrefix = emit->UseVEXEncoding() ? "v" : "";

    auto AddSuffix = [&](const char* insName, const char* suffix1, const char* suffix2 = "") -> const char* {
        const int       TEMP_BUFFER_LEN = 40;
        static unsigned curBuf          = 0;
        static char     buf[4][TEMP_BUFFER_LEN];
        const char*     retbuf;

        sprintf_s(buf[curBuf], TEMP_BUFFER_LEN, "%s%s%s", insName, suffix1, suffix2);
        retbuf = buf[curBuf];
        curBuf = (curBuf + 1) % 4;
        return retbuf;
    };

    auto RemoveVexPrefixIfNeeded = [&](const char* insName) -> const char* {
        if (emit->UseVEXEncoding())
        {
            return insName;
        }

        if (insName[0] == 'v')
        {
            return insName + 1;
        }

        return insName;
    };

    auto GetFltCmpOpName = [&](const char* suffix) -> const char* {
        static const char* const fltCmpOpNames[] = {
            "eq",    "lt",     "le",     "unord",    "neq",    "nlt",    "nle",    "ord",
            "eq_uq", "nge",    "ngt",    "false",    "neq_oq", "ge",     "gt",     "true",
            "eq_os", "lt_oq",  "le_oq",  "unord_s",  "neq_us", "nlt_uq", "nle_uq", "ord_s",
            "eq_us", "nge_uq", "ngt_uq", "false_os", "neq_os", "ge_oq",  "gt_oq",  "true_us",
        };

        uint8_t control = static_cast<uint8_t>(emit->emitGetInsSC(id));
        assert(control < ArrLen(fltCmpOpNames));

        const char* pseudoName = fltCmpOpNames[control];
        return RemoveVexPrefixIfNeeded(AddSuffix("vcmp", pseudoName, suffix));
    };

    auto GetIntCmpOpName = [&](const char* suffix) -> const char* {
        static const char* const intCmpOpNames[] = {
            "eq", "lt", "le", "neq", "false", "ge", "gt", "true",
        };

        uint8_t control = static_cast<uint8_t>(emit->emitGetInsSC(id));
        assert(control < ArrLen(intCmpOpNames));

        const char* pseudoName = intCmpOpNames[control];
        return RemoveVexPrefixIfNeeded(AddSuffix("vpcmp", pseudoName, suffix));
    };

    auto GetEvexOnlyName = [&](const char* pseudoName) -> const char* {
        if (emit->TakesEvexPrefix(id))
        {
            return pseudoName;
        }
        return RemoveVexPrefixIfNeeded(insName);
    };

    if (instHasPseudoName(ins))
    {
        // Some instructions have different mnemonics available

        switch (ins)
        {
            case INS_cdq:
            {
                switch (id->idOpSize())
                {
                    case EA_8BYTE:
                    {
                        return "cqo";
                    }

                    case EA_4BYTE:
                    {
                        return "cdq";
                    }

                    case EA_2BYTE:
                    {
                        return "cwd";
                    }

                    default:
                    {
                        unreached();
                    }
                }
                break;
            }

            case INS_cmppd:
            case INS_vcmppd:
            {
                return GetFltCmpOpName("pd");
            }

            case INS_cmpps:
            case INS_vcmpps:
            {
                return GetFltCmpOpName("ps");
            }

            case INS_cmpsd:
            case INS_vcmpsd:
            {
                return GetFltCmpOpName("sd");
            }

            case INS_cmpss:
            case INS_vcmpss:
            {
                return GetFltCmpOpName("ss");
            }

            case INS_cwde:
            {
                switch (id->idOpSize())
                {
                    case EA_8BYTE:
                    {
                        return "cdqe";
                    }

                    case EA_4BYTE:
                    {
                        return "cwde";
                    }

                    case EA_2BYTE:
                    {
                        return "cbw";
                    }

                    default:
                    {
                        unreached();
                    }
                }
            }

            case INS_movdqa32:
            {
                return GetEvexOnlyName("vmovdqa32");
            }

            case INS_movdqu32:
            {
                return GetEvexOnlyName("vmovdqu32");
            }

            case INS_pandd:
            {
                return GetEvexOnlyName("vpandd");
            }

            case INS_pandnd:
            {
                return GetEvexOnlyName("vpandnd");
            }

            case INS_pclmulqdq:
            {
                switch (static_cast<uint8_t>(emit->emitGetInsSC(id)) & 0x11)
                {
                    case 0x00:
                    {
                        return RemoveVexPrefixIfNeeded("vpclmullqlqdq");
                    }

                    case 0x01:
                    {
                        return RemoveVexPrefixIfNeeded("vpclmulhqlqdq");
                    }

                    case 0x10:
                    {
                        return RemoveVexPrefixIfNeeded("vpclmullqhqdq");
                    }

                    default:
                    {
                        return RemoveVexPrefixIfNeeded("vpclmulhqhqdq");
                    }
                }
            }

            case INS_pord:
            {
                return GetEvexOnlyName("vpord");
            }

            case INS_pxord:
            {
                return GetEvexOnlyName("vpxord");
            }

            case INS_roundpd:
            {
                return GetEvexOnlyName("vrndscalepd");
            }

            case INS_roundps:
            {
                return GetEvexOnlyName("vrndscaleps");
            }

            case INS_roundsd:
            {
                return GetEvexOnlyName("vrndscalesd");
            }

            case INS_roundss:
            {
                return GetEvexOnlyName("vrndscaless");
            }

            case INS_vbroadcastf32x4:
            {
                return GetEvexOnlyName("vbroadcastf32x4");
            }

            case INS_vbroadcasti32x4:
            {
                return GetEvexOnlyName("vbroadcasti32x4");
            }

            case INS_vextractf32x4:
            {
                return GetEvexOnlyName("vextractf32x4");
            }

            case INS_vextracti32x4:
            {
                return GetEvexOnlyName("vextracti32x4");
            }

            case INS_vinsertf32x4:
            {
                return GetEvexOnlyName("vinsertf32x4");
            }

            case INS_vinserti32x4:
            {
                return GetEvexOnlyName("vinserti32x4");
            }

            case INS_vpcmpb:
            {
                return GetIntCmpOpName("b");
            }

            case INS_vpcmpd:
            {
                return GetIntCmpOpName("d");
            }

            case INS_vpcmpq:
            {
                return GetIntCmpOpName("q");
            }

            case INS_vpcmpub:
            {
                return GetIntCmpOpName("ub");
            }

            case INS_vpcmpud:
            {
                return GetIntCmpOpName("ud");
            }

            case INS_vpcmpuq:
            {
                return GetIntCmpOpName("uq");
            }

            case INS_vpcmpuw:
            {
                return GetIntCmpOpName("uw");
            }

            case INS_vpcmpw:
            {
                return GetIntCmpOpName("w");
            }

            default:
            {
                unreached();
            }
        }
    }

    if (IsSimdInstruction(ins))
    {
        return RemoveVexPrefixIfNeeded(insName);
    }
    else if (id->idIsApxPpxContextSet())
    {
        return AddSuffix(insName, "p");
    }
#endif // TARGET_XARCH

    return insName;
}

/*****************************************************************************
 *
 *  Return the size string (e.g. "word ptr") appropriate for the given size.
 */

const char* CodeGen::genSizeStr(emitAttr attr)
{
    // clang-format off
    static
    const char * const sizes[] =
    {
        "byte  ptr ",
        "word  ptr ",
        "dword ptr ",
        "qword ptr ",
        "xmmword ptr ",
        "ymmword ptr ",
        "zmmword ptr "
    };
    // clang-format on

    unsigned size = EA_SIZE(attr);

    assert(genMaxOneBit(size) && (size <= 64));

    if (EA_ATTR(size) == attr)
    {
        return (size > 0) ? sizes[genLog2(size)] : "";
    }
    else if (attr == EA_GCREF)
    {
        return "gword ptr ";
    }
    else if (attr == EA_BYREF)
    {
        return "bword ptr ";
    }
    else if (EA_IS_DSP_RELOC(attr))
    {
        return "rword ptr ";
    }
    else
    {
        assert(!"Unexpected");
        return "unknw ptr ";
    }
}

/*****************************************************************************
 *
 *  Generate an instruction.
 */

void CodeGen::instGen(instruction ins)
{

    GetEmitter()->emitIns(ins);

#ifdef TARGET_XARCH
#ifdef PSEUDORANDOM_NOP_INSERTION
    // A workaround necessitated by limitations of emitter
    // if we are scheduled to insert a nop here, we have to delay it
    // hopefully we have not missed any other prefix instructions or places
    // they could be inserted
    if (ins == INS_lock && GetEmitter()->emitNextNop == 0)
    {
        GetEmitter()->emitNextNop = 1;
    }
#endif // PSEUDORANDOM_NOP_INSERTION
#endif
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is a floating-point ins.
 */

// static inline
bool CodeGenInterface::instIsFP(instruction ins)
{
    assert((unsigned)ins < ArrLen(instInfo));

#ifdef TARGET_XARCH
    return (instInfo[ins] & INS_FLAGS_x87Instr) != 0;
#else
    return (instInfo[ins] & INST_FP) != 0;
#endif
}

#if defined(TARGET_XARCH)
/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is an embedded broadcast
 *  compatible instruction.
 */

bool CodeGenInterface::instIsEmbeddedBroadcastCompatible(instruction ins)
{
    if (GetEmitter()->IsEvexEncodableInstruction(ins))
    {
        insTupleType tupleType = emitter::insTupleTypeInfo(ins);
        return (tupleType & INS_TT_IS_BROADCAST) != 0;
    }
    return false;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is an embedded masking
 *  compatible instruction.
 */

bool CodeGenInterface::instIsEmbeddedMaskingCompatible(instruction ins)
{
    return (ins != INS_invalid) && (instKMaskBaseSize(ins) != 0);
}

/*****************************************************************************
 *
 *  Returns the value of the given instruction's input size attribute, in bytes.
 */

unsigned CodeGenInterface::instInputSize(instruction ins)
{
    assert((unsigned)ins < ArrLen(instInfo));
    insFlags inputSize = static_cast<insFlags>((instInfo[ins] & Input_Mask));

    switch (inputSize)
    {
        case Input_8Bit:
            return 1;
        case Input_16Bit:
            return 2;
        case Input_32Bit:
            return 4;
        case Input_64Bit:
            return 8;
        default:
            unreached();
    }
}

/*****************************************************************************
 *
 *  Returns the value of the given instruction's KMask base size attribute, in bits.
 */

unsigned CodeGenInterface::instKMaskBaseSize(instruction ins)
{
    assert((unsigned)ins < ArrLen(instInfo));
    insFlags kmaskBaseSize = static_cast<insFlags>((instInfo[ins] & KMask_BaseMask));

    switch (kmaskBaseSize)
    {
        case KMask_Base1:
            return 1;
        case KMask_Base2:
            return 2;
        case KMask_Base4:
            return 4;
        case KMask_Base8:
            return 8;
        case KMask_Base16:
            return 16;
        default:
            return 0;
    }
}

/*****************************************************************************
 *
 *  Returns true if the given CPU instruction has a pseudo name
 */

bool CodeGenInterface::instHasPseudoName(instruction ins)
{
    assert((unsigned)ins < ArrLen(instInfo));
    return (instInfo[ins] & INS_FLAGS_HasPseudoName) != 0;
}
#endif // TARGET_XARCH

/*****************************************************************************
 *
 *  Generate a set instruction.
 */

void CodeGen::inst_SET(emitJumpKind condition, regNumber reg, insOpts instOptions)
{
#ifdef TARGET_XARCH
    instruction ins;

    /* Convert the condition to an instruction opcode */
    switch (condition)
    {
        case EJ_js:
            ins = INS_sets;
            break;
        case EJ_jns:
            ins = INS_setns;
            break;
        case EJ_je:
            ins = INS_sete;
            break;
        case EJ_jne:
            ins = INS_setne;
            break;

        case EJ_jl:
            ins = INS_setl;
            break;
        case EJ_jle:
            ins = INS_setle;
            break;
        case EJ_jge:
            ins = INS_setge;
            break;
        case EJ_jg:
            ins = INS_setg;
            break;

        case EJ_jb:
            ins = INS_setb;
            break;
        case EJ_jbe:
            ins = INS_setbe;
            break;
        case EJ_jae:
            ins = INS_setae;
            break;
        case EJ_ja:
            ins = INS_seta;
            break;

        case EJ_jp:
            ins = INS_setp;
            break;
        case EJ_jnp:
            ins = INS_setnp;
            break;

        default:
            NO_WAY("unexpected condition type");
            return;
    }

#ifdef TARGET_AMD64
    // If using ZU feature, we need to promote the SETcc to the new instruction.
    if ((instOptions & INS_OPTS_EVEX_zu_MASK) != 0)
    {
        const int offset = (INS_seto - INS_seto_apx);
        assert(INS_seto == (INS_seto_apx + offset));
        assert(INS_setno == (INS_setno_apx + offset));
        assert(INS_setb == (INS_setb_apx + offset));
        assert(INS_setae == (INS_setae_apx + offset));
        assert(INS_sete == (INS_sete_apx + offset));
        assert(INS_setne == (INS_setne_apx + offset));
        assert(INS_setbe == (INS_setbe_apx + offset));
        assert(INS_seta == (INS_seta_apx + offset));
        assert(INS_sets == (INS_sets_apx + offset));
        assert(INS_setns == (INS_setns_apx + offset));
        assert(INS_setp == (INS_setp_apx + offset));
        assert(INS_setnp == (INS_setnp_apx + offset));
        assert(INS_setl == (INS_setl_apx + offset));
        assert(INS_setge == (INS_setge_apx + offset));
        assert(INS_setle == (INS_setle_apx + offset));
        assert(INS_setg == (INS_setg_apx + offset));
        ins = (instruction)(ins - offset);
    }
#endif

    assert(genRegMask(reg) & RBM_BYTE_REGS);

    // These instructions only write the low byte of 'reg'
    GetEmitter()->emitIns_R(ins, EA_1BYTE, reg, instOptions);
#elif defined(TARGET_ARM64)

    GetEmitter()->emitIns_R_COND(INS_cset, EA_8BYTE, reg, JumpKindToInsCond(condition));
#else
    NYI("inst_SET");
#endif
}

/*****************************************************************************
 *
 *  Generate a "op reg" instruction.
 */

void CodeGen::inst_RV(instruction ins, regNumber reg, var_types type, emitAttr size)
{
    if (size == EA_UNKNOWN)
    {
        size = emitActualTypeSize(type);
    }

#ifdef TARGET_LOONGARCH64
    // inst_RV is not used for LoongArch64, so there is no need to define `emitIns_R`.
    NYI_LOONGARCH64("inst_RV-----unused on LOONGARCH64----");
#elif defined(TARGET_RISCV64)
    NYI_RISCV64("inst_RV-----unused on RISCV64----");
#else
    GetEmitter()->emitIns_R(ins, size, reg);
#endif
}

/*****************************************************************************
 *
 *  Generate a "mov reg1, reg2" instruction.
 */
void CodeGen::inst_Mov(var_types dstType,
                       regNumber dstReg,
                       regNumber srcReg,
                       bool      canSkip,
                       emitAttr  size,
                       insFlags  flags /* = INS_FLAGS_DONT_CARE */)
{
#if defined(TARGET_LOONGARCH64) || defined(TARGET_RISCV64)
    if (isFloatRegType(dstType) != genIsValidFloatReg(dstReg))
    {
        if (dstType == TYP_FLOAT)
        {
            dstType = TYP_INT;
        }
        else if (dstType == TYP_DOUBLE)
        {
            dstType = TYP_LONG;
        }
        else if (dstType == TYP_INT)
        {
            dstType = TYP_FLOAT;
        }
        else if (dstType == TYP_LONG)
        {
            dstType = TYP_DOUBLE;
        }
        else
        {
            NYI_LOONGARCH64("CodeGen::inst_Mov dstType");
            NYI_RISCV64("CodeGen::inst_Mov dstType");
        }
    }
#endif
    instruction ins = ins_Copy(srcReg, dstType);

    if (size == EA_UNKNOWN)
    {
        size = emitActualTypeSize(dstType);
    }

#ifdef TARGET_ARM
    GetEmitter()->emitIns_Mov(ins, size, dstReg, srcReg, canSkip, flags);
#else
    GetEmitter()->emitIns_Mov(ins, size, dstReg, srcReg, canSkip);
#endif
}

/*****************************************************************************
 *
 *  Generate a "mov reg1, reg2" instruction.
 */
void CodeGen::inst_Mov_Extend(var_types srcType,
                              bool      srcInReg,
                              regNumber dstReg,
                              regNumber srcReg,
                              bool      canSkip,
                              emitAttr  size,
                              insFlags  flags /* = INS_FLAGS_DONT_CARE */)
{
    instruction ins = ins_Move_Extend(srcType, srcInReg);

    if (size == EA_UNKNOWN)
    {
        size = emitActualTypeSize(srcType);
    }

#ifdef TARGET_ARM
    GetEmitter()->emitIns_Mov(ins, size, dstReg, srcReg, canSkip, flags);
#else
    GetEmitter()->emitIns_Mov(ins, size, dstReg, srcReg, canSkip);
#endif
}

/*****************************************************************************
 *
 *  Generate a "op reg1, reg2" instruction.
 */

//------------------------------------------------------------------------
// inst_RV_RV: Generate a "op reg1, reg2" instruction.
//
// Arguments:
//    ins   - the instruction to generate;
//    reg1  - the first register to use, the dst for most instructions;
//    reg2  - the second register to use, the src for most instructions;
//    type  - the type used to get the size attribute if not given, usually type of the reg2 operand;
//    size  - the size attribute, the type arg is ignored if this arg is provided with an actual value;
//    flags - whether flags are set for arm32.
//
void CodeGen::inst_RV_RV(instruction ins,
                         regNumber   reg1,
                         regNumber   reg2,
                         var_types   type /* = TYP_I_IMPL */,
                         emitAttr    size /* = EA_UNKNOWN */,
                         insFlags    flags /* = INS_FLAGS_DONT_CARE */)
{
    if (size == EA_UNKNOWN)
    {
        size = emitActualTypeSize(type);
    }

#ifdef TARGET_ARM
    GetEmitter()->emitIns_R_R(ins, size, reg1, reg2, flags);
#else
    GetEmitter()->emitIns_R_R(ins, size, reg1, reg2);
#endif
}

/*****************************************************************************
 *
 *  Generate a "op reg1, reg2, reg3" instruction.
 */

void CodeGen::inst_RV_RV_RV(instruction ins,
                            regNumber   reg1,
                            regNumber   reg2,
                            regNumber   reg3,
                            emitAttr    size,
                            insFlags    flags /* = INS_FLAGS_DONT_CARE */)
{
#ifdef TARGET_ARM
    GetEmitter()->emitIns_R_R_R(ins, size, reg1, reg2, reg3, flags);
#elif defined(TARGET_XARCH) || defined(TARGET_LOONGARCH64) || defined(TARGET_RISCV64)
    GetEmitter()->emitIns_R_R_R(ins, size, reg1, reg2, reg3);
#else
    NYI("inst_RV_RV_RV");
#endif
}
/*****************************************************************************
 *
 *  Generate a "op icon" instruction.
 */

void CodeGen::inst_IV(instruction ins, cnsval_ssize_t val)
{
    GetEmitter()->emitIns_I(ins, EA_PTRSIZE, val);
}

/*****************************************************************************
 *
 *  Generate a "op icon" instruction where icon is a handle of type specified
 *  by 'flags'
 */

void CodeGen::inst_IV_handle(instruction ins, cnsval_ssize_t val)
{
    GetEmitter()->emitIns_I(ins, EA_HANDLE_CNS_RELOC, val);
}

/*****************************************************************************
 *
 *  Display a stack frame reference.
 */

void CodeGen::inst_set_SV_var(GenTree* tree)
{
#ifdef DEBUG
    assert((tree != nullptr) && (tree->OperIs(GT_LCL_VAR, GT_STORE_LCL_VAR) || tree->IsLclVarAddr()));
    assert(tree->AsLclVarCommon()->GetLclNum() < compiler->lvaCount);

    GetEmitter()->emitVarRefOffs = tree->AsLclVar()->gtLclILoffs;

#endif // DEBUG
}

/*****************************************************************************
 *
 *  Generate a "op reg, icon" instruction.
 */

void CodeGen::inst_RV_IV(
    instruction ins, regNumber reg, target_ssize_t val, emitAttr size, insFlags flags /* = INS_FLAGS_DONT_CARE */)
{
#if !defined(TARGET_64BIT)
    assert(size != EA_8BYTE);
#endif

#ifdef TARGET_ARM
    if (arm_Valid_Imm_For_Instr(ins, val, flags))
    {
        GetEmitter()->emitIns_R_I(ins, size, reg, val, flags);
    }
    else if (ins == INS_mov)
    {
        instGen_Set_Reg_To_Imm(size, reg, val);
    }
    else
    {
        // TODO-Cleanup: Add a comment about why this is unreached() for RyuJIT backend.
        unreached();
    }
#elif defined(TARGET_ARM64)
    // TODO-Arm64-Bug: handle large constants!
    // Probably need something like the ARM case above: if (arm_Valid_Imm_For_Instr(ins, val)) ...
    assert(ins != INS_cmp);
    assert(ins != INS_tst);
    assert(ins != INS_mov);
    GetEmitter()->emitIns_R_R_I(ins, size, reg, reg, val);
#elif defined(TARGET_LOONGARCH64) || defined(TARGET_RISCV64)
    GetEmitter()->emitIns_R_R_I(ins, size, reg, reg, val);
#else // !TARGET_ARM
#ifdef TARGET_AMD64
    // Instead of an 8-byte immediate load, a 4-byte immediate will do fine
    // as the high 4 bytes will be zero anyway.
    if (size == EA_8BYTE && ins == INS_mov && ((val & 0xFFFFFFFF00000000LL) == 0))
    {
        size = EA_4BYTE;
        GetEmitter()->emitIns_R_I(ins, size, reg, val);
    }
    else if (EA_SIZE(size) == EA_8BYTE && ins != INS_mov && (((int)val != val) || EA_IS_CNS_RELOC(size)))
    {
        assert(!"Invalid immediate for inst_RV_IV");
    }
    else
#endif // TARGET_AMD64
    {
        GetEmitter()->emitIns_R_I(ins, size, reg, val);
    }
#endif // !TARGET_ARM
}

//------------------------------------------------------------------------
// inst_TT_RV: Generate a store of a lclVar
//
// Arguments:
//    ins  - the instruction to generate
//    size - the size attributes for the store
//    tree - the lclVar node
//    reg  - the register currently holding the value of the local
//
void CodeGen::inst_TT_RV(instruction ins, emitAttr size, GenTree* tree, regNumber reg)
{
#ifdef DEBUG
    // The tree must have a valid register value.
    assert(reg != REG_STK);

    bool isValidInReg = ((tree->gtFlags & GTF_SPILLED) == 0);
    if (!isValidInReg)
    {
        // Is this the special case of a write-thru lclVar?
        // We mark it as SPILLED to denote that its value is valid in memory.
        if (((tree->gtFlags & GTF_SPILL) != 0) && tree->OperIs(GT_STORE_LCL_VAR))
        {
            isValidInReg = true;
        }
    }
    assert(isValidInReg);
    assert(size != EA_UNKNOWN);
    assert(tree->OperIs(GT_LCL_VAR, GT_STORE_LCL_VAR));
#endif // DEBUG

    unsigned varNum = tree->AsLclVarCommon()->GetLclNum();
    assert(varNum < compiler->lvaCount);
#if CPU_LOAD_STORE_ARCH
#ifdef TARGET_ARM64
    // Workaround until https://github.com/dotnet/runtime/issues/105512 is fixed.
    assert(GetEmitter()->emitInsIsStore(ins) || (ins == INS_sve_str));
#else
    assert(GetEmitter()->emitInsIsStore(ins));
#endif

#endif
    GetEmitter()->emitIns_S_R(ins, size, reg, varNum, 0);
}

/*****************************************************************************
 *
 *  Generate a "shift reg, icon" instruction.
 */

void CodeGen::inst_RV_SH(
    instruction ins, emitAttr size, regNumber reg, unsigned val, insFlags flags /* = INS_FLAGS_DONT_CARE */)
{
#if defined(TARGET_ARM)

    if (val >= 32)
        val &= 0x1f;

    GetEmitter()->emitIns_R_I(ins, size, reg, val, flags);

#elif defined(TARGET_XARCH)

#ifdef TARGET_AMD64
    // X64 JB BE ensures only encodable values make it here.
    // x86 can encode 8 bits, though it masks down to 5 or 6
    // depending on 32-bit or 64-bit registers are used.
    // Here we will allow anything that is encodable.
    assert(val < 256);
#endif

    ins = genMapShiftInsToShiftByConstantIns(ins, val);

    if (val == 1)
    {
        GetEmitter()->emitIns_R(ins, size, reg);
    }
    else
    {
        GetEmitter()->emitIns_R_I(ins, size, reg, val);
    }

#else
    NYI("inst_RV_SH - unknown target");
#endif // TARGET*
}

#if defined(TARGET_XARCH)
//------------------------------------------------------------------------
// genOperandDesc: Create an operand descriptor for the given operand node.
//
// The XARCH emitter requires codegen to use different methods for different
// kinds of operands. However, the logic for determining which ones, in
// general, is not simple (due to the fact that "memory" in the emitter can
// be represented in more than one way). This helper method encapsulated the
// logic for determining what "kind" of operand "op" is.
//
// Arguments:
//    ins - The instruction that will consume the operand.
//    op - The operand node for which to obtain the descriptor.
//
// Return Value:
//    The operand descriptor for "op".
//
// Notes:
//    This method is not idempotent - it can only be called once for a
//    given node.
//
CodeGen::OperandDesc CodeGen::genOperandDesc(instruction ins, GenTree* op)
{
    if (!op->isContained() && !op->isUsedFromSpillTemp())
    {
        return OperandDesc(op->GetRegNum());
    }

    emitter* emit   = GetEmitter();
    TempDsc* tmpDsc = nullptr;
    unsigned varNum = BAD_VAR_NUM;
    uint16_t offset = UINT16_MAX;

    if (op->isUsedFromSpillTemp())
    {
        assert(op->IsRegOptional());

        tmpDsc = getSpillTempDsc(op);
        varNum = tmpDsc->tdTempNum();
        offset = 0;

        regSet.tmpRlsTemp(tmpDsc);
    }
    else if (op->isIndir() || op->OperIsHWIntrinsic())
    {
        GenTree*      addr;
        GenTreeIndir* memIndir = nullptr;

        if (op->isIndir())
        {
            memIndir = op->AsIndir();
            addr     = memIndir->Addr();
        }
        else
        {
            assert(op->OperIsHWIntrinsic());

#if defined(FEATURE_HW_INTRINSICS)
            GenTreeHWIntrinsic* hwintrinsic  = op->AsHWIntrinsic();
            NamedIntrinsic      intrinsicId  = hwintrinsic->GetHWIntrinsicId();
            var_types           simdBaseType = hwintrinsic->GetSimdBaseType();
            switch (intrinsicId)
            {
                case NI_SSE42_LoadAndDuplicateToVector128:
                case NI_AVX_BroadcastScalarToVector128:
                case NI_AVX_BroadcastScalarToVector256:
                {
                    // we have the assumption that these intrinsics
                    // only take a memory address as the operand.
                    assert(hwintrinsic->isContained());
                    assert(hwintrinsic->OperIsMemoryLoad());
                    assert(hwintrinsic->GetOperandCount() == 1);
                    assert(varTypeIsFloating(simdBaseType));
                    GenTree* hwintrinsicChild = hwintrinsic->Op(1);
                    assert(hwintrinsicChild->isContained());
                    if (hwintrinsicChild->OperIs(GT_LCL_ADDR, GT_CNS_INT, GT_LEA))
                    {
                        addr = hwintrinsic->Op(1);
                        break;
                    }
                    else
                    {
                        assert(hwintrinsicChild->OperIs(GT_LCL_VAR));
                        return OperandDesc(simdBaseType, hwintrinsicChild);
                    }
                }

                case NI_SSE42_MoveAndDuplicate:
                case NI_AVX2_BroadcastScalarToVector128:
                case NI_AVX2_BroadcastScalarToVector256:
                case NI_AVX512_BroadcastScalarToVector512:
                {
                    assert(hwintrinsic->isContained());
                    if (intrinsicId == NI_SSE42_MoveAndDuplicate)
                    {
                        assert(simdBaseType == TYP_DOUBLE);
                    }
                    // If broadcast node is contained, should mean that we have some forms like
                    // Broadcast -> CreateScalarUnsafe -> Scalar.
                    // If so, directly emit scalar.
                    // In the code below, we specially handle the `Broadcast -> CNS_INT/CNS_LNG` form and
                    // handle other cases recursively.
                    GenTree* hwintrinsicChild = hwintrinsic->Op(1);
                    assert(hwintrinsicChild->isContained());
                    if (hwintrinsicChild->IsIntegralConst())
                    {
                        // a special case is when the operand of CreateScalarUnsafe is an integer type,
                        // CreateScalarUnsafe node will be folded, so we directly match a pattern of
                        // broadcast -> LCL_VAR(TYP_(U)INT/LONG)
                        INT64          scalarValue = hwintrinsicChild->AsIntConCommon()->IntegralValue();
                        UNATIVE_OFFSET cnum        = emit->emitDataConst(&scalarValue, genTypeSize(simdBaseType),
                                                                         genTypeSize(simdBaseType), simdBaseType);
                        return OperandDesc(compiler->eeFindJitDataOffs(cnum));
                    }
                    else
                    {
                        // If the operand of broadcast is not a constant integer,
                        // we handle all the other cases recursively.
                        return genOperandDesc(ins, hwintrinsicChild);
                    }
                    break;
                }

                case NI_Vector128_CreateScalar:
                case NI_Vector256_CreateScalar:
                case NI_Vector512_CreateScalar:
                case NI_Vector128_CreateScalarUnsafe:
                case NI_Vector256_CreateScalarUnsafe:
                case NI_Vector512_CreateScalarUnsafe:
                {
                    // The hwintrinsic should be contained and its
                    // op1 should be either contained or spilled. This
                    // allows us to transparently "look through" the
                    // CreateScalar/Unsafe and treat it directly like
                    // a load from memory.

                    assert(hwintrinsic->isContained());
                    op = hwintrinsic->Op(1);
                    return genOperandDesc(ins, op);
                }

                default:
                {
                    assert(hwintrinsic->OperIsMemoryLoad());
                    assert(hwintrinsic->GetOperandCount() == 1);

                    addr = hwintrinsic->Op(1);
                    break;
                }
            }
#else
            unreached();
#endif // FEATURE_HW_INTRINSICS
        }

        if (addr->isContained() && addr->OperIs(GT_LCL_ADDR))
        {
            varNum = addr->AsLclFld()->GetLclNum();
            offset = addr->AsLclFld()->GetLclOffs();
        }
        else
        {
            return (memIndir != nullptr) ? OperandDesc(memIndir) : OperandDesc(op->TypeGet(), addr);
        }
    }
    else
    {
        switch (op->OperGet())
        {
            case GT_LCL_FLD:
                varNum = op->AsLclFld()->GetLclNum();
                offset = op->AsLclFld()->GetLclOffs();
                break;

            case GT_LCL_VAR:
                assert(op->IsRegOptional() || !compiler->lvaGetDesc(op->AsLclVar())->lvIsRegCandidate());
                varNum = op->AsLclVar()->GetLclNum();
                offset = 0;
                break;

            case GT_CNS_DBL:
                return OperandDesc(emit->emitFltOrDblConst(op->AsDblCon()->DconValue(), emitTypeSize(op)));

            case GT_CNS_INT:
            {
                assert(op->isContainedIntOrIImmed());
                return OperandDesc(op->AsIntCon()->IconValue(), op->AsIntCon()->ImmedValNeedsReloc(compiler));
            }

#if defined(FEATURE_SIMD)
            case GT_CNS_VEC:
            {
                insTupleType tupleType = emitter::insTupleTypeInfo(ins);
                unsigned     cnsSize   = genTypeSize(op);

                if ((tupleType == INS_TT_TUPLE1_SCALAR) || (tupleType == INS_TT_TUPLE1_FIXED))
                {
                    // We have a vector const, but the instruction will only read a scalar from it,
                    // so don't waste space putting the entire vector to the data section.

                    cnsSize = max(CodeGenInterface::instInputSize(ins), 4U);
                    assert(cnsSize <= genTypeSize(op));
                }

                return OperandDesc(emit->emitSimdConst(&op->AsVecCon()->gtSimdVal, EA_TYPE(cnsSize)));
            }
#endif // FEATURE_SIMD

#if defined(FEATURE_MASKED_HW_INTRINSICS)
            case GT_CNS_MSK:
            {
                return OperandDesc(emit->emitSimdMaskConst(op->AsMskCon()->gtSimdMaskVal));
            }
#endif // FEATURE_MASKED_HW_INTRINSICS

            default:
                unreached();
        }
    }

    // Ensure we got a good varNum and offset.
    // We also need to check for `tmpDsc != nullptr` since spill temp numbers
    // are negative and start with -1, which also happens to be BAD_VAR_NUM.
    assert((varNum != BAD_VAR_NUM) || (tmpDsc != nullptr));
    assert(offset != UINT16_MAX);

    return OperandDesc(varNum, offset);
}

//------------------------------------------------------------------------
// inst_TT: Generates an instruction with one operand.
//
// Arguments:
//    ins       -- The instruction being emitted
//    size      -- The emit size attribute
//    op1       -- The operand, which may be a memory node or a node producing a register,
//                 or a contained immediate node
//
void CodeGen::inst_TT(instruction ins, emitAttr size, GenTree* op1)
{
    emitter*    emit    = GetEmitter();
    OperandDesc op1Desc = genOperandDesc(ins, op1);

    switch (op1Desc.GetKind())
    {
        case OperandKind::ClsVar:
            emit->emitIns_C(ins, size, op1Desc.GetFieldHnd(), 0);
            break;

        case OperandKind::Local:
            emit->emitIns_S(ins, size, op1Desc.GetVarNum(), op1Desc.GetLclOffset());
            break;

        case OperandKind::Indir:
        {
            // Until we improve the handling of addressing modes in the emitter, we'll create a
            // temporary GT_IND to generate code with.
            GenTreeIndir  indirForm;
            GenTreeIndir* indir = op1Desc.GetIndirForm(&indirForm);
            emit->emitIns_A(ins, size, indir);
        }
        break;

        case OperandKind::Imm:
            emit->emitIns_I(ins, op1Desc.GetEmitAttrForImmediate(size), op1Desc.GetImmediate());
            break;

        case OperandKind::Reg:
            emit->emitIns_R(ins, size, op1Desc.GetReg());
            break;

        default:
            unreached();
    }
}

//------------------------------------------------------------------------
// inst_RV_TT: Generates an instruction with two operands, the first of which
//             is a register.
//
// Arguments:
//    ins       -- The instruction being emitted
//    size      -- The emit size attribute
//    op1Reg    -- The first operand, a register
//    op2       -- The operand, which may be a memory node or a node producing a register,
//                 or a contained immediate node
//
void CodeGen::inst_RV_TT(instruction ins, emitAttr size, regNumber op1Reg, GenTree* op2)
{
    emitter*    emit    = GetEmitter();
    OperandDesc op2Desc = genOperandDesc(ins, op2);

    switch (op2Desc.GetKind())
    {
        case OperandKind::ClsVar:
            emit->emitIns_R_C(ins, size, op1Reg, op2Desc.GetFieldHnd(), 0);
            break;

        case OperandKind::Local:
            emit->emitIns_R_S(ins, size, op1Reg, op2Desc.GetVarNum(), op2Desc.GetLclOffset());
            break;

        case OperandKind::Indir:
        {
            // Until we improve the handling of addressing modes in the emitter, we'll create a
            // temporary GT_IND to generate code with.
            GenTreeIndir  indirForm;
            GenTreeIndir* indir = op2Desc.GetIndirForm(&indirForm);
            emit->emitIns_R_A(ins, size, op1Reg, indir);
        }
        break;

        case OperandKind::Imm:
            emit->emitIns_R_I(ins, op2Desc.GetEmitAttrForImmediate(size), op1Reg, op2Desc.GetImmediate());
            break;

        case OperandKind::Reg:
            if (emit->IsMovInstruction(ins))
            {
                emit->emitIns_Mov(ins, size, op1Reg, op2Desc.GetReg(), /* canSkip */ true);
            }
            else
            {
                emit->emitIns_R_R(ins, size, op1Reg, op2Desc.GetReg());
            }
            break;

        default:
            unreached();
    }
}

/*****************************************************************************
 *
 *  Generate an instruction of the form "op reg1, reg2, icon".
 */

void CodeGen::inst_RV_RV_IV(instruction ins, emitAttr size, regNumber reg1, regNumber reg2, unsigned ival)
{
    assert(ins == INS_shld || ins == INS_shrd || ins == INS_shufps || ins == INS_shufpd || ins == INS_pshufd ||
           ins == INS_cmpps || ins == INS_cmppd || ins == INS_dppd || ins == INS_dpps || ins == INS_insertps ||
           ins == INS_roundps || ins == INS_roundss || ins == INS_roundpd || ins == INS_roundsd);

    GetEmitter()->emitIns_R_R_I(ins, size, reg1, reg2, ival);
}

//------------------------------------------------------------------------
// inst_RV_TT_IV: Generates an instruction that takes 3 operands:
//                a register operand, an operand that may be memory or register and an immediate
//                and that returns a value in register
//
// Arguments:
//    ins          -- The instruction being emitted
//    attr         -- The emit attribute
//    reg1         -- The first operand, a register
//    rmOp         -- The second operand, which may be a memory node or a node producing a register
//    ival         -- The immediate operand
//    instOptions  -- The options that modify how the instruction is generated
//
void CodeGen::inst_RV_TT_IV(
    instruction ins, emitAttr attr, regNumber reg1, GenTree* rmOp, int ival, insOpts instOptions)
{
    emitter* emit = GetEmitter();
    noway_assert(emit->emitVerifyEncodable(ins, EA_SIZE(attr), reg1));

#if defined(TARGET_XARCH) && defined(FEATURE_HW_INTRINSICS)
    if (CodeGenInterface::IsEmbeddedBroadcastEnabled(ins, rmOp))
    {
        instOptions = AddEmbBroadcastMode(instOptions);
    }
    else if ((instOptions == INS_OPTS_NONE) && !GetEmitter()->IsVexEncodableInstruction(ins))
    {
        // We may have opportunistically selected an EVEX only instruction
        // that isn't actually required, so fallback to the VEX compatible
        // encoding to potentially save on the number of bytes emitted.

        switch (ins)
        {
            case INS_vextractf64x2:
            {
                ins = INS_vextractf32x4;
                break;
            }

            case INS_vextracti64x2:
            {
                ins = INS_vextracti32x4;
                break;
            }

            default:
            {
                break;
            }
        }
    }
#endif // TARGET_XARCH && FEATURE_HW_INTRINSICS

    OperandDesc rmOpDesc = genOperandDesc(ins, rmOp);

    switch (rmOpDesc.GetKind())
    {
        case OperandKind::ClsVar:
            emit->emitIns_R_C_I(ins, attr, reg1, rmOpDesc.GetFieldHnd(), 0, ival, instOptions);
            break;

        case OperandKind::Local:
            emit->emitIns_R_S_I(ins, attr, reg1, rmOpDesc.GetVarNum(), rmOpDesc.GetLclOffset(), ival, instOptions);
            break;

        case OperandKind::Indir:
        {
            // Until we improve the handling of addressing modes in the emitter, we'll create a
            // temporary GT_IND to generate code with.
            GenTreeIndir  indirForm;
            GenTreeIndir* indir = rmOpDesc.GetIndirForm(&indirForm);
            emit->emitIns_R_A_I(ins, attr, reg1, indir, ival, instOptions);
        }
        break;

        case OperandKind::Reg:
            emit->emitIns_SIMD_R_R_I(ins, attr, reg1, rmOpDesc.GetReg(), ival, instOptions);
            break;

        default:
            unreached();
    }
}

//------------------------------------------------------------------------
// IsEmbeddedBroadcastEnabled: determine if embedded broadcast can be enabled
//
// Arguments:
//    ins       -- The instruction being emitted
//    op        -- The second operand of the instruction.
//
#if defined(TARGET_XARCH) && defined(FEATURE_HW_INTRINSICS)
bool CodeGenInterface::IsEmbeddedBroadcastEnabled(instruction ins, GenTree* op)
{
    // To enable embedded broadcast, we need 3 things,
    // 1. EVEX enabled.
    // 2. Embedded broadcast compatible intrinsics
    // 3. A contained broadcast scalar node

    if (!GetEmitter()->UseEvexEncoding())
    {
        return false;
    }

    if (!instIsEmbeddedBroadcastCompatible(ins))
    {
        return false;
    }

    if (!op->isContained() || !op->OperIsHWIntrinsic())
    {
        return false;
    }

    return op->AsHWIntrinsic()->OperIsBroadcastScalar();
}

//------------------------------------------------------------------------
// AddEmbBroadcastMode: Adds the embedded broadcast mode to the insOpts
//
// Arguments:
//    instOptions - The existing insOpts
//
// Return Value:
//    The modified insOpts
//
insOpts CodeGen::AddEmbBroadcastMode(insOpts instOptions)
{
    assert((instOptions & INS_OPTS_EVEX_b_MASK) == 0);
    unsigned result = static_cast<unsigned>(instOptions);
    return static_cast<insOpts>(result | INS_OPTS_EVEX_eb);
}
#endif //  TARGET_XARCH && FEATURE_HW_INTRINSICS

//------------------------------------------------------------------------
// inst_RV_RV_TT: Generates an instruction that takes 2 operands:
//                a register operand and an operand that may be in memory or register
//                the result is returned in register
//
// Arguments:
//    ins          -- The instruction being emitted
//    size         -- The emit size attribute
//    targetReg    -- The target register
//    op1Reg       -- The first operand register
//    op2          -- The second operand, which may be a memory node or a node producing a register
//    isRMW        -- true if the instruction is RMW; otherwise, false
//    instOptions  -- The options that modify how the instruction is generated
void CodeGen::inst_RV_RV_TT(instruction ins,
                            emitAttr    size,
                            regNumber   targetReg,
                            regNumber   op1Reg,
                            GenTree*    op2,
                            bool        isRMW,
                            insOpts     instOptions)
{
    emitter* emit = GetEmitter();
    noway_assert(emit->emitVerifyEncodable(ins, EA_SIZE(size), targetReg));

    // TODO-XArch-CQ: Commutative operations can have op1 be contained
    // TODO-XArch-CQ: Non-VEX encoded instructions can have both ops contained

#if defined(TARGET_XARCH) && defined(FEATURE_HW_INTRINSICS)
    if (CodeGenInterface::IsEmbeddedBroadcastEnabled(ins, op2))
    {
        instOptions = AddEmbBroadcastMode(instOptions);
    }
    else if ((instOptions == INS_OPTS_NONE) && !GetEmitter()->IsVexEncodableInstruction(ins))
    {
        // We may have opportunistically selected an EVEX only instruction
        // that isn't actually required, so fallback to the VEX compatible
        // encoding to potentially save on the number of bytes emitted.

        switch (ins)
        {
            case INS_vpandq:
            {
                ins = INS_pandd;
                break;
            }

            case INS_vpandnq:
            {
                ins = INS_pandnd;
                break;
            }

            case INS_vporq:
            {
                ins = INS_pord;
                break;
            }

            case INS_vpxorq:
            {
                ins = INS_pxord;
                break;
            }

            default:
            {
                break;
            }
        }
    }
#endif // TARGET_XARCH && FEATURE_HW_INTRINSICS

    OperandDesc op2Desc = genOperandDesc(ins, op2);

    switch (op2Desc.GetKind())
    {
        case OperandKind::ClsVar:
        {
            emit->emitIns_SIMD_R_R_C(ins, size, targetReg, op1Reg, op2Desc.GetFieldHnd(), 0, instOptions);
            break;
        }
        case OperandKind::Local:
            emit->emitIns_SIMD_R_R_S(ins, size, targetReg, op1Reg, op2Desc.GetVarNum(), op2Desc.GetLclOffset(),
                                     instOptions);
            break;

        case OperandKind::Indir:
        {
            // Until we improve the handling of addressing modes in the emitter, we'll create a
            // temporary GT_IND to generate code with.
            GenTreeIndir  indirForm;
            GenTreeIndir* indir = op2Desc.GetIndirForm(&indirForm);
            emit->emitIns_SIMD_R_R_A(ins, size, targetReg, op1Reg, indir, instOptions);
        }
        break;

        case OperandKind::Reg:
        {
            regNumber op2Reg = op2Desc.GetReg();

            if ((op1Reg != targetReg) && (op2Reg == targetReg) && isRMW)
            {
                // We have "reg2 = reg1 op reg2" where "reg1 != reg2" on a RMW instruction.
                //
                // For non-commutative instructions, we should have ensured that op2 was marked
                // delay free in order to prevent it from getting assigned the same register
                // as target. However, for commutative instructions, we can just swap the operands
                // in order to have "reg2 = reg2 op reg1" which will end up producing the right code.

                op2Reg = op1Reg;
                op1Reg = targetReg;
            }

            emit->emitIns_SIMD_R_R_R(ins, size, targetReg, op1Reg, op2Reg, instOptions);
        }
        break;

        default:
            unreached();
    }
}

//------------------------------------------------------------------------
// inst_RV_RV_TT_IV: Generates an instruction that takes 3 operands:
//                   a register operand, an operand that may be in memory or register,
//                   and an immediate value. The result is returned in register
//
// Arguments:
//    ins          -- The instruction being emitted
//    size         -- The emit size attribute
//    targetReg    -- The target register
//    op1Reg       -- The first operand register
//    op2          -- The second operand, which may be a memory node or a node producing a register
//    ival         -- The immediate operand
//    isRMW        -- true if the instruction is RMW; otherwise, false
//    instOptions  -- The options that modify how the instruction is generated
//
void CodeGen::inst_RV_RV_TT_IV(instruction ins,
                               emitAttr    size,
                               regNumber   targetReg,
                               regNumber   op1Reg,
                               GenTree*    op2,
                               int8_t      ival,
                               bool        isRMW,
                               insOpts     instOptions)
{
    emitter* emit = GetEmitter();
    noway_assert(emit->emitVerifyEncodable(ins, EA_SIZE(size), op1Reg));

    // TODO-XArch-CQ: Commutative operations can have op1 be contained
    // TODO-XArch-CQ: Non-VEX encoded instructions can have both ops contained

#if defined(TARGET_XARCH) && defined(FEATURE_HW_INTRINSICS)
    if (CodeGenInterface::IsEmbeddedBroadcastEnabled(ins, op2))
    {
        instOptions = AddEmbBroadcastMode(instOptions);
    }
    else if ((instOptions == INS_OPTS_NONE) && !GetEmitter()->IsVexEncodableInstruction(ins))
    {
        // We may have opportunistically selected an EVEX only instruction
        // that isn't actually required, so fallback to the VEX compatible
        // encoding to potentially save on the number of bytes emitted.

        switch (ins)
        {
            case INS_vinsertf64x2:
            {
                ins = INS_vinsertf32x4;
                break;
            }

            case INS_vinserti64x2:
            {
                ins = INS_vinserti32x4;
                break;
            }

            default:
            {
                break;
            }
        }
    }
#endif // TARGET_XARCH && FEATURE_HW_INTRINSICS

    OperandDesc op2Desc = genOperandDesc(ins, op2);

    switch (op2Desc.GetKind())
    {
        case OperandKind::ClsVar:
            emit->emitIns_SIMD_R_R_C_I(ins, size, targetReg, op1Reg, op2Desc.GetFieldHnd(), 0, ival, instOptions);
            break;

        case OperandKind::Local:
            emit->emitIns_SIMD_R_R_S_I(ins, size, targetReg, op1Reg, op2Desc.GetVarNum(), op2Desc.GetLclOffset(), ival,
                                       instOptions);
            break;

        case OperandKind::Indir:
        {
            // Until we improve the handling of addressing modes in the emitter, we'll create a
            // temporary GT_IND to generate code with.
            GenTreeIndir  indirForm;
            GenTreeIndir* indir = op2Desc.GetIndirForm(&indirForm);
            emit->emitIns_SIMD_R_R_A_I(ins, size, targetReg, op1Reg, indir, ival, instOptions);
        }
        break;

        case OperandKind::Reg:
        {
            regNumber op2Reg = op2Desc.GetReg();

            if ((op1Reg != targetReg) && (op2Reg == targetReg) && isRMW)
            {
                // We have "reg2 = reg1 op reg2" where "reg1 != reg2" on a RMW intrinsic.
                //
                // For non-commutative intrinsics, we should have ensured that op2 was marked
                // delay free in order to prevent it from getting assigned the same register
                // as target. However, for commutative intrinsics, we can just swap the operands
                // in order to have "reg2 = reg2 op reg1" which will end up producing the right code.

                op2Reg = op1Reg;
                op1Reg = targetReg;
            }

            emit->emitIns_SIMD_R_R_R_I(ins, size, targetReg, op1Reg, op2Reg, ival, instOptions);
        }
        break;

        default:
            unreached();
    }
}
#endif // TARGET_XARCH

/*****************************************************************************
 *
 *  The following should all end up inline in compiler.hpp at some point.
 */

void CodeGen::inst_ST_RV(instruction ins, TempDsc* tmp, unsigned ofs, regNumber reg, var_types type)
{
    GetEmitter()->emitIns_S_R(ins, emitActualTypeSize(type), reg, tmp->tdTempNum(), ofs);
}

#ifdef TARGET_XARCH
void CodeGen::inst_FS_ST(instruction ins, emitAttr size, TempDsc* tmp, unsigned ofs)
{
    GetEmitter()->emitIns_S(ins, size, tmp->tdTempNum(), ofs);
}
#endif

#ifdef TARGET_ARM
bool CodeGenInterface::validImmForInstr(instruction ins, target_ssize_t imm, insFlags flags)
{
    if (GetEmitter()->emitInsIsLoadOrStore(ins) && !instIsFP(ins))
    {
        return validDispForLdSt(imm, TYP_INT);
    }

    bool result = false;
    switch (ins)
    {
        case INS_cmp:
        case INS_cmn:
            if (validImmForAlu(imm) || validImmForAlu(-imm))
                result = true;
            break;

        case INS_and:
        case INS_bic:
        case INS_orr:
        case INS_orn:
        case INS_mvn:
            if (validImmForAlu(imm) || validImmForAlu(~imm))
                result = true;
            break;

        case INS_mov:
            if (validImmForMov(imm))
                result = true;
            break;

        case INS_addw:
        case INS_subw:
            if ((unsigned_abs(imm) <= 0x00000fff) && (flags != INS_FLAGS_SET)) // 12-bit immediate
                result = true;
            break;

        case INS_add:
        case INS_sub:
            if (validImmForAdd(imm, flags))
                result = true;
            break;

        case INS_tst:
        case INS_eor:
        case INS_teq:
        case INS_adc:
        case INS_sbc:
        case INS_rsb:
            if (validImmForAlu(imm))
                result = true;
            break;

        case INS_asr:
        case INS_lsl:
        case INS_lsr:
        case INS_ror:
            if (imm > 0 && imm <= 32)
                result = true;
            break;

        case INS_vstr:
        case INS_vldr:
            if ((imm & 0x3FC) == imm)
                result = true;
            break;

        default:
            break;
    }
    return result;
}
bool CodeGen::arm_Valid_Imm_For_Instr(instruction ins, target_ssize_t imm, insFlags flags)
{
    return validImmForInstr(ins, imm, flags);
}

bool CodeGenInterface::validDispForLdSt(target_ssize_t disp, var_types type)
{
    if (varTypeIsFloating(type))
    {
        if ((disp & 0x3FC) == disp)
            return true;
        else
            return false;
    }
    else
    {
        if ((disp >= -0x00ff) && (disp <= 0x0fff))
            return true;
        else
            return false;
    }
}

bool CodeGenInterface::validImmForAlu(target_ssize_t imm)
{
    return emitter::emitIns_valid_imm_for_alu(imm);
}

bool CodeGenInterface::validImmForMov(target_ssize_t imm)
{
    return emitter::emitIns_valid_imm_for_mov(imm);
}

bool CodeGenInterface::validImmForAdd(target_ssize_t imm, insFlags flags)
{
    return emitter::emitIns_valid_imm_for_add(imm, flags);
}

bool CodeGen::arm_Valid_Imm_For_Add(target_ssize_t imm, insFlags flags)
{
    return emitter::emitIns_valid_imm_for_add(imm, flags);
}

// Check "add Rd,SP,i10"
bool CodeGen::arm_Valid_Imm_For_Add_SP(target_ssize_t imm)
{
    return emitter::emitIns_valid_imm_for_add_sp(imm);
}

bool CodeGenInterface::validImmForBL(ssize_t addr)
{
    return
        // If we are running the altjit for AOT, then assume we can use the "BL" instruction.
        // This matches the usual behavior for AOT, since we normally do generate "BL".
        (!compiler->info.compMatchedVM && compiler->IsAot()) ||
        (compiler->eeGetRelocTypeHint((void*)addr) == IMAGE_REL_BASED_THUMB_BRANCH24);
}

#endif // TARGET_ARM

#if defined(TARGET_ARM64)
bool CodeGenInterface::validImmForBL(ssize_t addr)
{
    // On arm64, we always assume a call target is in range and generate a 28-bit relative
    // 'bl' instruction. If this isn't sufficient range, the VM will generate a jump stub when
    // we call recordRelocation(). See the IMAGE_REL_ARM64_BRANCH26 case in jitinterface.cpp
    // (for JIT) or zapinfo.cpp (for AOT). If we cannot allocate a jump stub, it is fatal.
    return true;
}
#endif // TARGET_ARM64

/*****************************************************************************
 *
 *  Get the machine dependent instruction for performing sign/zero extension.
 *
 *  Parameters
 *      srcType   - source type
 *      srcInReg  - whether source is in a register
 */
instruction CodeGen::ins_Move_Extend(var_types srcType, bool srcInReg)
{
    if (varTypeUsesIntReg(srcType))
    {
        instruction ins = INS_invalid;

#if defined(TARGET_XARCH)
        if (!varTypeIsSmall(srcType))
        {
            ins = INS_mov;
        }
        else if (varTypeIsUnsigned(srcType))
        {
            ins = INS_movzx;
        }
        else
        {
            ins = INS_movsx;
        }
#elif defined(TARGET_ARM)
        //
        // Register to Register zero/sign extend operation
        //
        if (srcInReg)
        {
            if (!varTypeIsSmall(srcType))
            {
                ins = INS_mov;
            }
            else if (varTypeIsUnsigned(srcType))
            {
                if (varTypeIsByte(srcType))
                    ins = INS_uxtb;
                else
                    ins = INS_uxth;
            }
            else
            {
                if (varTypeIsByte(srcType))
                    ins = INS_sxtb;
                else
                    ins = INS_sxth;
            }
        }
        else
        {
            ins = ins_Load(srcType);
        }
#elif defined(TARGET_ARM64)
        //
        // Register to Register zero/sign extend operation
        //
        if (srcInReg)
        {
            if (varTypeIsUnsigned(srcType))
            {
                if (varTypeIsByte(srcType))
                {
                    ins = INS_uxtb;
                }
                else if (varTypeIsShort(srcType))
                {
                    ins = INS_uxth;
                }
                else
                {
                    // A mov Rd, Rm instruction performs the zero extend
                    // for the upper 32 bits when the size is EA_4BYTE

                    ins = INS_mov;
                }
            }
            else
            {
                if (varTypeIsByte(srcType))
                {
                    ins = INS_sxtb;
                }
                else if (varTypeIsShort(srcType))
                {
                    ins = INS_sxth;
                }
                else
                {
                    if (srcType == TYP_INT)
                    {
                        ins = INS_sxtw;
                    }
                    else
                    {
                        ins = INS_mov;
                    }
                }
            }
        }
        else
        {
            ins = ins_Load(srcType);
        }
#else
        NYI("ins_Move_Extend");
#endif

        assert(ins != INS_invalid);
        return ins;
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(srcType))
    {
#if defined(TARGET_XARCH)
        return INS_kmovq_msk;
#elif defined(TARGET_ARM64)
        return INS_sve_mov;
#endif
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(srcType));

#if defined(TARGET_XARCH)
    // SSE2/AVX requires destination to be a reg always.
    // If src is in reg means, it is a reg-reg move.
    //
    // SSE2 Note: always prefer movaps/movups over movapd/movupd since the
    // former doesn't require 66h prefix and one byte smaller than the
    // latter.
    //
    // TODO-CQ: based on whether src type is aligned use movaps instead

    if (srcInReg)
    {
        return INS_movaps;
    }

    unsigned srcSize = genTypeSize(srcType);

    if (srcSize == 4)
    {
        return INS_movss;
    }
    else if (srcSize == 8)
    {
        return INS_movsd_simd;
    }
    else
    {
        assert((srcSize == 12) || (srcSize == 16) || (srcSize == 32) || (srcSize == 64));
        return INS_movups;
    }
#elif defined(TARGET_ARM64)
    return (srcInReg) ? INS_mov : ins_Load(srcType);
#elif defined(TARGET_ARM)
    assert(!varTypeIsSIMD(srcType));
    return INS_vmov;
#else
    NYI("ins_Move_Extend");
    return INS_invalid;
#endif
}

/*****************************************************************************
 *
 *  Get the machine dependent instruction for performing a load for srcType
 *
 *  Parameters
 *      srcType   - source type
 *      aligned   - whether source is properly aligned if srcType is a SIMD type
 */
instruction CodeGenInterface::ins_Load(var_types srcType, bool aligned /*=false*/)
{
    if (varTypeUsesIntReg(srcType))
    {
        instruction ins = INS_invalid;

#if defined(TARGET_XARCH)
        if (!varTypeIsSmall(srcType))
        {
            ins = INS_mov;
        }
        else if (varTypeIsUnsigned(srcType))
        {
            ins = INS_movzx;
        }
        else
        {
            ins = INS_movsx;
        }
#elif defined(TARGET_ARMARCH)
        if (!varTypeIsSmall(srcType))
        {
            ins = INS_ldr;
        }
        else if (varTypeIsByte(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_ldrb;
            else
                ins = INS_ldrsb;
        }
        else if (varTypeIsShort(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_ldrh;
            else
                ins = INS_ldrsh;
        }
#elif defined(TARGET_LOONGARCH64)
        if (varTypeIsByte(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_ld_bu;
            else
                ins = INS_ld_b;
        }
        else if (varTypeIsShort(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_ld_hu;
            else
                ins = INS_ld_h;
        }
        else if (TYP_INT == srcType)
        {
            ins = INS_ld_w;
        }
        else
        {
            ins = INS_ld_d; // default ld_d.
        }
#elif defined(TARGET_RISCV64)
        if (varTypeIsByte(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_lbu;
            else
                ins = INS_lb;
        }
        else if (varTypeIsShort(srcType))
        {
            if (varTypeIsUnsigned(srcType))
                ins = INS_lhu;
            else
                ins = INS_lh;
        }
        else if (TYP_INT == srcType)
        {
            ins = INS_lw;
        }
        else
        {
            ins = INS_ld; // default ld.
        }
#else
        NYI("ins_Load");
#endif

        assert(ins != INS_invalid);
        return ins;
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(srcType))
    {
#if defined(TARGET_XARCH)
        return INS_kmovq_msk;
#elif defined(TARGET_ARM64)
        return INS_sve_ldr;
#endif
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(srcType));

#if defined(TARGET_XARCH)
    unsigned srcSize = genTypeSize(srcType);

    if (srcSize == 4)
    {
        return INS_movss;
    }
    else if (srcSize == 8)
    {
        return INS_movsd_simd;
    }
    else
    {
        assert((srcSize == 12) || (srcSize == 16) || (srcSize == 32) || (srcSize == 64));

        // SSE2 Note: always prefer movaps/movups over movapd/movupd since the
        // former doesn't require 66h prefix and one byte smaller than the
        // latter.

        return (aligned) ? INS_movaps : INS_movups;
    }
#elif defined(TARGET_ARM64)
    return INS_ldr;
#elif defined(TARGET_ARM)
    assert(!varTypeIsSIMD(srcType));
    return INS_vldr;
#elif defined(TARGET_LOONGARCH64)
    assert(!varTypeIsSIMD(srcType));

    if (srcType == TYP_DOUBLE)
    {
        return INS_fld_d;
    }
    else
    {
        assert(srcType == TYP_FLOAT);
        return INS_fld_s;
    }
#elif defined(TARGET_RISCV64)
    assert(!varTypeIsSIMD(srcType));

    if (srcType == TYP_DOUBLE)
    {
        return INS_fld;
    }
    else
    {
        assert(srcType == TYP_FLOAT);
        return INS_flw;
    }
#else
    NYI("ins_Load");
#endif
}

/*****************************************************************************
 *
 *  Get the machine dependent instruction for performing a reg-reg copy for dstType
 *
 *  Parameters
 *      dstType   - destination type
 */
instruction CodeGen::ins_Copy(var_types dstType)
{
    assert(emitTypeActSz[dstType] != 0);

    if (varTypeUsesIntReg(dstType))
    {
#if defined(TARGET_XARCH) || defined(TARGET_ARMARCH) || defined(TARGET_LOONGARCH64) || defined(TARGET_RISCV64)
        return INS_mov;
#else
        NYI("ins_Copy");
#endif
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(dstType))
    {
#if defined(TARGET_XARCH)
        return INS_kmovq_msk;
#elif defined(TARGET_ARM64)
        return INS_sve_mov;
#endif
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(dstType));

#if defined(TARGET_XARCH)
    return INS_movaps;
#elif defined(TARGET_ARM64)
    if (varTypeIsSIMD(dstType))
    {
        return INS_mov;
    }
    else
    {
        assert(varTypeIsFloating(dstType));
        return INS_fmov;
    }
#elif defined(TARGET_ARM)
    assert(!varTypeIsSIMD(dstType));
    return INS_vmov;
#elif defined(TARGET_LOONGARCH64)
    assert(!varTypeIsSIMD(dstType));

    if (dstType == TYP_DOUBLE)
    {
        return INS_fmov_d;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return INS_fmov_s;
    }
#elif defined(TARGET_RISCV64)
    assert(!varTypeIsSIMD(dstType));

    if (dstType == TYP_DOUBLE)
    {
        return INS_fsgnj_d;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return INS_fsgnj_s;
    }
#else
    NYI("ins_Copy");
#endif
}

//------------------------------------------------------------------------
//  ins_Copy: Get the machine dependent instruction for performing a reg-reg copy
//            from srcReg to a register of dstType.
//
// Arguments:
//      srcReg  - source register
//      dstType - destination type
//
// Notes:
//    This assumes the size of the value in 'srcReg' is the same as the size of
//    'dstType'.
//
instruction CodeGen::ins_Copy(regNumber srcReg, var_types dstType)
{
    assert(srcReg != REG_NA);

    if (varTypeUsesIntReg(dstType))
    {
        if (genIsValidIntOrFakeReg(srcReg))
        {
            // int to int
            return ins_Copy(dstType);
        }

#if defined(FEATURE_MASKED_HW_INTRINSICS) && defined(TARGET_XARCH)
        if (genIsValidMaskReg(srcReg))
        {
            // mask to int
            return INS_kmovq_gpr;
        }
#endif // FEATURE_MASKED_HW_INTRINSICS && TARGET_XARCH

        // float to int
        assert(genIsValidFloatReg(srcReg));

#if defined(TARGET_AMD64)
        return EA_SIZE(emitActualTypeSize(dstType)) == EA_4BYTE ? INS_movd32 : INS_movd64;
#elif defined(TARGET_X86)
        return INS_movd32;
#elif defined(TARGET_ARM64)
        return INS_mov;
#elif defined(TARGET_ARM)
        // Can't have LONG in a register.
        assert(dstType == TYP_INT);

        assert(!varTypeIsSIMD(dstType));
        return INS_vmov_f2i;
#elif defined(TARGET_LOONGARCH64)
        assert(!varTypeIsSIMD(dstType));
        return EA_SIZE(emitActualTypeSize(dstType)) == EA_4BYTE ? INS_movfr2gr_s : INS_movfr2gr_d;
#elif defined(TARGET_RISCV64)
        assert(!varTypeIsSIMD(dstType));
        return EA_SIZE(emitActualTypeSize(dstType)) == EA_4BYTE ? INS_fmv_x_w : INS_fmv_x_d;
#else
        NYI("ins_Copy");
#endif
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(dstType))
    {
        if (genIsValidMaskReg(srcReg))
        {
            // mask to mask
            return ins_Copy(dstType);
        }

        // mask to int
        assert(genIsValidIntOrFakeReg(srcReg));
#if defined(TARGET_XARCH)
        return INS_kmovq_gpr;
#elif defined(TARGET_ARM64)
        return INS_sve_mov;
#endif
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(dstType));

    if (genIsValidFloatReg(srcReg))
    {
        // float to float
        return ins_Copy(dstType);
    }

    // int to float
    assert(genIsValidIntOrFakeReg(srcReg));

#if defined(TARGET_AMD64)
    return EA_SIZE(emitActualTypeSize(dstType)) == EA_4BYTE ? INS_movd32 : INS_movd64;
#elif defined(TARGET_X86)
    return INS_movd32;
#elif defined(TARGET_ARM64)
    return INS_fmov;
#elif defined(TARGET_ARM)
    // Can't have LONG in a register.
    assert(dstType == TYP_FLOAT);

    assert(!varTypeIsSIMD(dstType));
    return INS_vmov_i2f;
#elif defined(TARGET_LOONGARCH64)
    assert(!varTypeIsSIMD(dstType));

    if (dstType == TYP_DOUBLE)
    {
        return INS_movgr2fr_d;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return INS_movgr2fr_w;
    }
#elif defined(TARGET_RISCV64)
    assert(!varTypeIsSIMD(dstType));
    assert(!genIsValidFloatReg(srcReg));

    if (dstType == TYP_DOUBLE)
    {
        return INS_fmv_d_x;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return INS_fmv_w_x;
    }
#else
    NYI("ins_Copy");
#endif
}

/*****************************************************************************
 *
 *  Get the machine dependent instruction for performing a store for dstType
 *
 *  Parameters
 *      dstType   - destination type
 *      aligned   - whether destination is properly aligned if dstType is a SIMD type
 *                - for LoongArch64 aligned is used for store-index.
 */
instruction CodeGenInterface::ins_Store(var_types dstType, bool aligned /*=false*/)
{
    if (varTypeUsesIntReg(dstType))
    {
        instruction ins = INS_invalid;

#if defined(TARGET_XARCH)
        ins = INS_mov;
#elif defined(TARGET_ARMARCH)
        if (!varTypeIsSmall(dstType))
            ins = INS_str;
        else if (varTypeIsByte(dstType))
            ins = INS_strb;
        else if (varTypeIsShort(dstType))
            ins = INS_strh;
#elif defined(TARGET_LOONGARCH64)
        if (varTypeIsByte(dstType))
            ins = aligned ? INS_stx_b : INS_st_b;
        else if (varTypeIsShort(dstType))
            ins = aligned ? INS_stx_h : INS_st_h;
        else if (TYP_INT == dstType)
            ins = aligned ? INS_stx_w : INS_st_w;
        else
            ins = aligned ? INS_stx_d : INS_st_d;
#elif defined(TARGET_RISCV64)
        if (varTypeIsByte(dstType))
            ins = INS_sb;
        else if (varTypeIsShort(dstType))
            ins = INS_sh;
        else if (TYP_INT == dstType)
            ins = INS_sw;
        else
            ins = INS_sd;
#else
        NYI("ins_Store");
#endif
        assert(ins != INS_invalid);
        return ins;
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(dstType))
    {
#if defined(TARGET_XARCH)
        return INS_kmovq_msk;
#elif defined(TARGET_ARM64)
        return INS_sve_str;
#endif
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(dstType));

#if defined(TARGET_XARCH)
    unsigned dstSize = genTypeSize(dstType);

    if (dstSize == 4)
    {
        return INS_movss;
    }
    else if (dstSize == 8)
    {
        return INS_movsd_simd;
    }
    else
    {
        assert((dstSize == 12) || (dstSize == 16) || (dstSize == 32) || (dstSize == 64));

        // SSE2 Note: always prefer movaps/movups over movapd/movupd since the
        // former doesn't require 66h prefix and one byte smaller than the
        // latter.

        return (aligned) ? INS_movaps : INS_movups;
    }
#elif defined(TARGET_ARM64)
    return INS_str;
#elif defined(TARGET_ARM)
    assert(!varTypeIsSIMD(dstType));
    return INS_vstr;
#elif defined(TARGET_LOONGARCH64)
    assert(!varTypeIsSIMD(dstType));

    if (dstType == TYP_DOUBLE)
    {
        return aligned ? INS_fstx_d : INS_fst_d;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return aligned ? INS_fstx_s : INS_fst_s;
    }
#elif defined(TARGET_RISCV64)
    assert(!varTypeIsSIMD(dstType));

    if (dstType == TYP_DOUBLE)
    {
        return INS_fsd;
    }
    else
    {
        assert(dstType == TYP_FLOAT);
        return INS_fsw;
    }
#else
    NYI("ins_Store");
#endif
}

//------------------------------------------------------------------------
// ins_StoreFromSrc: Get the machine dependent instruction for performing a store to dstType on the stack from a srcReg.
//
// Arguments:
//   srcReg  - the source register for the store
//   dstType - the destination type
//   aligned - whether the destination is properly aligned if dstType is a SIMD type
//
// Return Value:
//   the instruction to use
//
instruction CodeGenInterface::ins_StoreFromSrc(regNumber srcReg, var_types dstType, bool aligned /*=false*/)
{
    assert(srcReg != REG_NA);

    if (varTypeUsesIntReg(dstType))
    {
        if (genIsValidIntOrFakeReg(srcReg))
        {
            // int to int
            return ins_Store(dstType, aligned);
        }

#if defined(TARGET_XARCH) && defined(FEATURE_SIMD)
        if (genIsValidMaskReg(srcReg))
        {
            // mask to int, treat as mask so it works on 32-bit
            return ins_Store(TYP_MASK, aligned);
        }
#endif // TARGET_XARCH && FEATURE_SIMD

        // float to int, treat as float to float
        assert(genIsValidFloatReg(srcReg));

        unsigned dstSize = genTypeSize(dstType);

        if (dstSize == 4)
        {
            dstType = TYP_FLOAT;
        }
        else
        {
#if defined(TARGET_64BIT)
            assert(dstSize == 8);
            dstType = TYP_DOUBLE;
#else
            unreached();
#endif
        }

        return ins_Store(dstType, aligned);
    }

#if defined(FEATURE_MASKED_HW_INTRINSICS)
    if (varTypeUsesMaskReg(dstType))
    {
        if (genIsValidMaskReg(srcReg))
        {
            // mask to mask
            return ins_Store(dstType, aligned);
        }

        // mask to int, keep as mask so it works on 32-bit
        assert(genIsValidIntOrFakeReg(srcReg));
        return ins_Store(dstType, aligned);
    }
#endif // FEATURE_MASKED_HW_INTRINSICS

    assert(varTypeUsesFloatReg(dstType));

    if (genIsValidIntOrFakeReg(srcReg))
    {
        // int to float, treat as int to int

        unsigned dstSize = genTypeSize(dstType);

        if (dstSize == 4)
        {
            dstType = TYP_INT;
        }
        else
        {
#if defined(TARGET_64BIT)
            assert(dstSize == 8);
            dstType = TYP_LONG;
#else
            unreached();
#endif
        }
    }
    else
    {
        // float to float
        assert(genIsValidFloatReg(srcReg));
    }

    return ins_Store(dstType, aligned);
}

#if defined(TARGET_XARCH)

instruction CodeGen::ins_MathOp(genTreeOps oper, var_types type)
{
    switch (oper)
    {
        case GT_ADD:
            return type == TYP_DOUBLE ? INS_addsd : INS_addss;
        case GT_SUB:
            return type == TYP_DOUBLE ? INS_subsd : INS_subss;
        case GT_MUL:
            return type == TYP_DOUBLE ? INS_mulsd : INS_mulss;
        case GT_DIV:
            return type == TYP_DOUBLE ? INS_divsd : INS_divss;
        default:
            unreached();
    }
}

//------------------------------------------------------------------------
// ins_FloatConv: Conversions to or from floating point values.
//
// Arguments:
//    to - Destination type.
//    from - Source type.
//
// Returns:
//    The correct conversion instruction to use based on src and dst types.
//
instruction CodeGen::ins_FloatConv(var_types to, var_types from)
{
    // AVX: Supports following conversions
    //   srcType = int16/int64                     castToType = float
    // AVX512: Supports following conversions
    //   srcType = ulong                           castToType = double/float
    bool isAvx10v2 = false;
    switch (from)
    {
        case TYP_INT:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_cvtsi2ss32;
                case TYP_DOUBLE:
                    return INS_cvtsi2sd32;
                default:
                    unreached();
            }
            break;

        case TYP_LONG:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_cvtsi2ss64;
                case TYP_DOUBLE:
                    return INS_cvtsi2sd64;
                default:
                    unreached();
            }
            break;

        case TYP_UINT:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_vcvtusi2ss32;
                case TYP_DOUBLE:
                    return INS_vcvtusi2sd32;
                default:
                    unreached();
            }
            break;

        case TYP_ULONG:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_vcvtusi2ss64;
                case TYP_DOUBLE:
                    return INS_vcvtusi2sd64;
                default:
                    unreached();
            }
            break;

        case TYP_FLOAT:
            if (to == TYP_FLOAT)
            {
                return ins_Move_Extend(TYP_FLOAT, false);
            }
            else if (to == TYP_DOUBLE)
            {
                return INS_cvtss2sd;
            }
            isAvx10v2 = compiler->compOpportunisticallyDependsOn(InstructionSet_AVX10v2);

            switch (to)
            {
                case TYP_INT:
                    return isAvx10v2 ? INS_vcvttss2sis32 : INS_cvttss2si32;
                case TYP_LONG:
                    return isAvx10v2 ? INS_vcvttss2sis64 : INS_cvttss2si64;
                case TYP_ULONG:
                    return isAvx10v2 ? INS_vcvttss2usis64 : INS_vcvttss2usi64;
                case TYP_UINT:
                    return isAvx10v2 ? INS_vcvttss2usis32 : INS_vcvttss2usi32;
                default:
                    unreached();
            }
            break;

        case TYP_DOUBLE:
            if (to == TYP_FLOAT)
            {
                return INS_cvtsd2ss;
            }
            else if (to == TYP_DOUBLE)
            {
                return ins_Move_Extend(TYP_DOUBLE, false);
            }
            isAvx10v2 = compiler->compOpportunisticallyDependsOn(InstructionSet_AVX10v2);

            switch (to)
            {
                case TYP_INT:
                    return isAvx10v2 ? INS_vcvttsd2sis32 : INS_cvttsd2si32;
                case TYP_LONG:
                    return isAvx10v2 ? INS_vcvttsd2sis64 : INS_cvttsd2si64;
                case TYP_ULONG:
                    return isAvx10v2 ? INS_vcvttsd2usis64 : INS_vcvttsd2usi64;
                case TYP_UINT:
                    return isAvx10v2 ? INS_vcvttsd2usis32 : INS_vcvttsd2usi32;
                default:
                    unreached();
            }
            break;

        default:
            unreached();
    }
}

#elif defined(TARGET_ARM)

instruction CodeGen::ins_MathOp(genTreeOps oper, var_types type)
{
    switch (oper)
    {
        case GT_ADD:
            return INS_vadd;
        case GT_SUB:
            return INS_vsub;
        case GT_MUL:
            return INS_vmul;
        case GT_DIV:
            return INS_vdiv;
        case GT_NEG:
            return INS_vneg;
        default:
            unreached();
    }
}

instruction CodeGen::ins_FloatConv(var_types to, var_types from)
{
    switch (from)
    {
        case TYP_INT:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_vcvt_i2f;
                case TYP_DOUBLE:
                    return INS_vcvt_i2d;
                default:
                    unreached();
            }
            break;
        case TYP_UINT:
            switch (to)
            {
                case TYP_FLOAT:
                    return INS_vcvt_u2f;
                case TYP_DOUBLE:
                    return INS_vcvt_u2d;
                default:
                    unreached();
            }
            break;
        case TYP_LONG:
            switch (to)
            {
                case TYP_FLOAT:
                    NYI("long to float");
                    break;
                case TYP_DOUBLE:
                    NYI("long to double");
                    break;
                default:
                    unreached();
            }
            break;
        case TYP_FLOAT:
            switch (to)
            {
                case TYP_INT:
                    return INS_vcvt_f2i;
                case TYP_UINT:
                    return INS_vcvt_f2u;
                case TYP_LONG:
                    NYI("float to long");
                    break;
                case TYP_DOUBLE:
                    return INS_vcvt_f2d;
                case TYP_FLOAT:
                    return INS_vmov;
                default:
                    unreached();
            }
            break;
        case TYP_DOUBLE:
            switch (to)
            {
                case TYP_INT:
                    return INS_vcvt_d2i;
                case TYP_UINT:
                    return INS_vcvt_d2u;
                case TYP_LONG:
                    NYI("double to long");
                    break;
                case TYP_FLOAT:
                    return INS_vcvt_d2f;
                case TYP_DOUBLE:
                    return INS_vmov;
                default:
                    unreached();
            }
            break;
        default:
            unreached();
    }
    unreached();
}

#endif // TARGET_ARM

/*****************************************************************************
 *
 *  Machine independent way to return
 */
void CodeGen::instGen_Return(unsigned stkArgSize)
{
#if defined(TARGET_XARCH)
    if (stkArgSize == 0)
    {
        instGen(INS_ret);
    }
    else
    {
        inst_IV(INS_ret, stkArgSize);
    }
#elif defined(TARGET_ARM)
//
// The return on ARM is folded into the pop multiple instruction
// and as we do not know the exact set of registers that we will
// need to restore (pop) when we first call instGen_Return we will
// instead just not emit anything for this method on the ARM
// The return will be part of the pop multiple and that will be
// part of the epilog that is generated by genFnEpilog()
#elif defined(TARGET_ARM64)
    // This function shouldn't be used on ARM64.
    unreached();
#else
    NYI("instGen_Return");
#endif
}

/*****************************************************************************
 *
 *  Machine independent way to move a Zero value into a register
 */
void CodeGen::instGen_Set_Reg_To_Zero(emitAttr size, regNumber reg, insFlags flags)
{
    assert(genIsValidIntOrFakeReg(reg));

#if defined(TARGET_XARCH)
    GetEmitter()->emitIns_R_R(INS_xor, size, reg, reg);
#elif defined(TARGET_ARM)
    GetEmitter()->emitIns_R_I(INS_mov, size, reg, 0 ARM_ARG(flags));
#elif defined(TARGET_ARM64)
    GetEmitter()->emitIns_Mov(INS_mov, size, reg, REG_ZR, /* canSkip */ true);
#elif defined(TARGET_LOONGARCH64)
    GetEmitter()->emitIns_R_R_I(INS_ori, size, reg, REG_R0, 0);
#elif defined(TARGET_RISCV64)
    GetEmitter()->emitIns_R_R_I(INS_addi, size, reg, REG_R0, 0);
#else
#error "Unknown TARGET"
#endif

    regSet.verifyRegUsed(reg);
}

#if defined(TARGET_AMD64)
//------------------------------------------------------------------------
// instGen_Push2Pop2Ppx: Generate push2/pop2 with ppx hint on.
//
// Arguments:
//    ins - The instruction to generate (push or pop).
//    reg1 - The first register to push/pop.
//    reg2 - The second register to push/pop.
//
void CodeGen::instGen_Push2Pop2Ppx(instruction ins, regNumber reg1, regNumber reg2)
{
    GetEmitter()->emitIns_R_R(ins, EA_PTRSIZE, reg1, reg2, (insOpts)(INS_OPTS_EVEX_nd | INS_OPTS_APX_ppx));
}
#endif // defined(TARGET_AMD64)

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
