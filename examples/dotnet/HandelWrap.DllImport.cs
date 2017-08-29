using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

/*
* Copyright (c) 2014 XIA LLC
* All rights reserved
*
* Redistribution and use in source and binary forms,
* with or without modification, are permitted provided
* that the following conditions are met:
*
*   * Redistributions of source code must retain the above
*     copyright notice, this list of conditions and the
*     following disclaimer.
*   * Redistributions in binary form must reproduce the
*     above copyright notice, this list of conditions and the
*     following disclaimer in the documentation and/or other
*     materials provided with the distribution.
*   * Neither the name of XIA LLC
*     nor the names of its contributors may be used to endorse
*     or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
* THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

namespace XIA.Components
{
    public partial class HandelWrap
    {
        //
        // Exported functions from Handel
        // Overload functions instead of using void pointers.
        // All Handel functions will return an int
        //
        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetNumModules(ref int nModule);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetDetectors_VB(int nModuleIndex, StringBuilder name);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetModules_VB(int nModuleIndex, StringBuilder name);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetModuleItem(string alias, string name, ref int value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetModuleItem(string alias, string name, ref uint value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetModuleItem(string alias, string name, StringBuilder value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaModifyModuleItem(string alias, string name, ref int value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetDetectorItem(string alias, string name, StringBuilder value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetFirmwareItem(string alias, ushort ptrr, string name, StringBuilder value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaInit(string iniFile);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaLoadSystem(string name, string iniFile);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaStartSystem();

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSaveSystem(string name, string iniFile);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaExit();

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGainOperation(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, ref ushort value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, ref uint value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, ref int value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, uint[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, int[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetRunData(int detChan, string name, double[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, ref uint value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, ref int value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, double[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, byte[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, StringBuilder value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, ref ushort value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaBoardOperation(int detChan, string name, ref char value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetAcquisitionValues(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        protected static extern int xiaSetAcquisitionValues(int detChan, string name, StringBuilder svalue);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSetAcquisitionValues(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        protected static extern int xiaSetAcquisitionValues(int detChan, string name, byte[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaRemoveAcquisitionValues(int detChan, string name);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetNumParams(int detChan, ref ushort numParams);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetParamName(int detChan, ushort index, StringBuilder name);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetParamData(int detChan, string name, ushort[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetParameter(int detChan, string name, ref ushort value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSetParameter(int detChan, string name, ushort value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaDoSpecialRun(int detChan, string name, double[] info);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, ref int value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, ref double value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, uint[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, int[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, double[] value);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetSpecialRunData(int detChan, string name, StringBuilder text);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaStartRun(int detChan, int clearMCA);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaStopRun(int detChan);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaGetVersionInfo(int rel, int min, int maj, StringBuilder pretty);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSetIOPriority(int pri);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSetLogLevel(int level);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSetLogOutput(string fileName);

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaSuppressLogOutput();

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaEnableLogOutput();

        [DllImport("handel.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int xiaCloseLog();
    }
}
