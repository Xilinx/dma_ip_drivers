/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#pragma once

/** Product Version (common.ver resources compatible) ***********************/

#undef  VER_PRODUCTMAJORVERSION
#define VER_PRODUCTMAJORVERSION            (2020)
#undef  VER_PRODUCTMINORVERSION
#define VER_PRODUCTMINORVERSION            (2)
#undef  VER_PRODUCTREVISION
#define VER_PRODUCTREVISION                (0)
#undef  VER_PRODUCTBUILD
#define VER_PRODUCTBUILD                   (0)

/** Company Details *********************************************************/

#undef  VER_COMPANYNAME_STR
#define VER_COMPANYNAME_STR                "Xilinx, Inc."

/** Copyright Details *******************************************************/

#undef  VER_LEGALCOPYRIGHT_YEARS
#define VER_LEGALCOPYRIGHT_YEARS           "2019-2020"
#undef  VER_LEGALCOPYRIGHT_STR_WITH_YEARS
#define VER_LEGALCOPYRIGHT_STR_WITH_YEARS  \
    "Copyright 2019-2020 Xilinx, Inc."
#undef  VER_LEGALCOPYRIGHT_STR
#if defined(RC_INVOKED)
#define VER_LEGALCOPYRIGHT_STR             \
    "\251 Copyright Xilinx, Inc. All rights reserved."
#else
#define VER_LEGALCOPYRIGHT_STR             \
    "(c) Copyright Xilinx, Inc. All rights reserved."
#endif

/** Product Details *********************************************************/

#undef  VER_PRODUCTNAME_STR
#define VER_PRODUCTNAME_STR                \
    "Xilinx PCIe Multi-Queue-DMA Reference Driver"

// Version number (in format needed for version resources)
#undef  VER_PRODUCTVERSION
#define VER_PRODUCTVERSION                 2020,2,0,0

// Version number string
#undef  VER_PRODUCTVERSION_STR
#define VER_PRODUCTVERSION_STR             "2020.2.0.0"

/** File Details ************************************************************/

// Default file version to be the same as product version
#define VER_FILEVERSION                    VER_PRODUCTVERSION
#define VER_FILEVERSION_STR                VER_PRODUCTVERSION_STR

// Default to language-independent software
#define VER_LANGNEUTRAL

// Don't append a build machine tag to the file version string
#undef  __BUILDMACHINE__
