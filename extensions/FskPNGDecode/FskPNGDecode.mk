<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<makefile>

<include name="/makefiles/xsFskDefaults.mk"/>

<input name="$(F_HOME)/kinoma/kinoma-ipp-lib/"/>

<wrap name="FskPNGDecodeExtension.c"/>
<wrap name="FskPngDecode.c"/>

<platform name="android">
<common>
OBJECTS += $(TMP_DIR)/png_arm_v7.gas7.o
$(TMP_DIR)/png_arm_v7.gas7.o:  $(F_HOME)/extensions/FskPNGDecode/sources/png_arm_v7.gas7
	$(deviceAS) $(F_HOME)/extensions/FskPNGDecode/sources/png_arm_v7.gas7 -o $(TMP_DIR)/png_arm_v7.gas7.o
</common>
</platform>

<platform name="linux/bg3cdp,linux/guitar,linux/chip,linux/nanopim1">
<common>
OBJECTS += $(TMP_DIR)/png_arm_v7.gas7.o
$(TMP_DIR)/png_arm_v7.gas7.o:  $(F_HOME)/extensions/FskPNGDecode/sources/png_arm_v7.gas7
	$(deviceAS) $(F_HOME)/extensions/FskPNGDecode/sources/png_arm_v7.gas7 -o $(TMP_DIR)/png_arm_v7.gas7.o
</common>
</platform>

<platform name="iphone/device">
<wrap name="png_arm_v7.gas7"/>
</platform>

<platform name="iphone/simulator">
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>
