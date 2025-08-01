﻿// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using Microsoft.Diagnostics.DataContractReader.Contracts;
using Microsoft.Diagnostics.DataContractReader.Contracts.StackWalkHelpers;

namespace Microsoft.Diagnostics.DataContractReader.Legacy;

[GeneratedComClass]
internal sealed unsafe partial class ClrDataStackWalk : IXCLRDataStackWalk
{
    private readonly TargetPointer _threadAddr;
    private readonly uint _flags;
    private readonly Target _target;
    private readonly IXCLRDataStackWalk? _legacyImpl;

    private bool _currentFrameIsValid;
    private readonly IEnumerator<IStackDataFrameHandle> _dataFrames;

    public ClrDataStackWalk(TargetPointer threadAddr, uint flags, Target target, IXCLRDataStackWalk? legacyImpl)
    {
        _threadAddr = threadAddr;
        _flags = flags;
        _target = target;
        _legacyImpl = legacyImpl;

        ThreadData threadData = _target.Contracts.Thread.GetThreadData(_threadAddr);
        _dataFrames = _target.Contracts.StackWalk.CreateStackWalk(threadData).GetEnumerator();

        // IEnumerator<T> begins before the first element.
        // Call MoveNext() to set _dataFrames.Current to the first element.
        _currentFrameIsValid = _dataFrames.MoveNext();
    }

    int IXCLRDataStackWalk.GetContext(uint contextFlags, uint contextBufSize, uint* contextSize, [MarshalUsing(CountElementName = "contextBufSize"), Out] byte[] contextBuf)
    {
        int hr = HResults.S_OK;

        if (_currentFrameIsValid)
        {
            IStackWalk sw = _target.Contracts.StackWalk;
            IStackDataFrameHandle dataFrame = _dataFrames.Current;
            byte[] context = sw.GetRawContext(dataFrame);
            if (context.Length > contextBufSize)
                hr = HResults.E_INVALIDARG;

            if (contextSize is not null)
            {
                *contextSize = (uint)context.Length;
            }

            context.CopyTo(contextBuf);
        }
        else
        {
            hr = HResults.S_FALSE;
        }


#if DEBUG
        if (_legacyImpl is not null)
        {
            byte[] localContextBuf = new byte[contextBufSize];
            int hrLocal = _legacyImpl.GetContext(contextFlags, contextBufSize, null, localContextBuf);
            Debug.Assert(hrLocal == hr, $"cDAC: {hr:x}, DAC: {hrLocal:x}");

            if (hr == HResults.S_OK)
            {
                IPlatformAgnosticContext contextStruct = IPlatformAgnosticContext.GetContextForPlatform(_target);
                IPlatformAgnosticContext localContextStruct = IPlatformAgnosticContext.GetContextForPlatform(_target);
                contextStruct.FillFromBuffer(contextBuf);
                localContextStruct.FillFromBuffer(localContextBuf);

                Debug.Assert(contextStruct.Equals(localContextStruct));
            }
        }
#endif

        return hr;
    }

    int IXCLRDataStackWalk.GetFrame(void** frame)
        => _legacyImpl is not null ? _legacyImpl.GetFrame(frame) : HResults.E_NOTIMPL;
    int IXCLRDataStackWalk.GetFrameType(uint* simpleType, uint* detailedType)
        => _legacyImpl is not null ? _legacyImpl.GetFrameType(simpleType, detailedType) : HResults.E_NOTIMPL;
    int IXCLRDataStackWalk.GetStackSizeSkipped(ulong* stackSizeSkipped)
        => _legacyImpl is not null ? _legacyImpl.GetStackSizeSkipped(stackSizeSkipped) : HResults.E_NOTIMPL;
    int IXCLRDataStackWalk.Next()
    {
        int hr;
        try
        {
            _currentFrameIsValid = _dataFrames.MoveNext();
            hr = _currentFrameIsValid ? HResults.S_OK : HResults.S_FALSE;
        }
        catch (System.Exception ex)
        {
            hr = ex.HResult;
        }

#if DEBUG
        if (_legacyImpl is not null)
        {
            int hrLocal = _legacyImpl.Next();
            Debug.Assert(hrLocal == hr, $"cDAC: {hr:x}, DAC: {hrLocal:x}");
        }
#endif

        return hr;
    }
    int IXCLRDataStackWalk.Request(uint reqCode, uint inBufferSize, byte* inBuffer, uint outBufferSize, byte* outBuffer)
    {
        const uint DACSTACKPRIV_REQUEST_FRAME_DATA = 0xf0000000;

        int hr = HResults.S_OK;

        switch (reqCode)
        {
            case DACSTACKPRIV_REQUEST_FRAME_DATA:
                if (outBufferSize < sizeof(ulong))
                    hr = HResults.E_INVALIDARG;

                IStackWalk sw = _target.Contracts.StackWalk;
                IStackDataFrameHandle frameData = _dataFrames.Current;
                TargetPointer frameAddr = sw.GetFrameAddress(frameData);
                *(ulong*)outBuffer = frameAddr.ToClrDataAddress(_target);
                hr = HResults.S_OK;
                break;
            default:
                hr = HResults.E_NOTIMPL;
                break;
        }

#if DEBUG
        if (_legacyImpl is not null)
        {
            int hrLocal;
            byte[] localOutBuffer = new byte[outBufferSize];
            fixed (byte* localOutBufferPtr = localOutBuffer)
            {
                hrLocal = _legacyImpl.Request(reqCode, inBufferSize, inBuffer, outBufferSize, localOutBufferPtr);
            }
            Debug.Assert(hrLocal == hr, $"cDAC: {hr:x}, DAC: {hrLocal:x}");

            for (int i = 0; i < outBufferSize; i++)
            {
                Debug.Assert(localOutBuffer[i] == outBuffer[i], $"cDAC: {outBuffer[i]:x}, DAC: {localOutBuffer[i]:x}");
            }
        }
#endif
        return hr;
    }
    int IXCLRDataStackWalk.SetContext(uint contextSize, [In, MarshalUsing(CountElementName = "contextSize")] byte[] context)
        => _legacyImpl is not null ? _legacyImpl.SetContext(contextSize, context) : HResults.E_NOTIMPL;
    int IXCLRDataStackWalk.SetContext2(uint flags, uint contextSize, [In, MarshalUsing(CountElementName = "contextSize")] byte[] context)
        => _legacyImpl is not null ? _legacyImpl.SetContext2(flags, contextSize, context) : HResults.E_NOTIMPL;
}
