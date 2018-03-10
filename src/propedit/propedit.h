/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/os.h"

#include "common/common_pch.h"

#include "common/doc_type_version_handler.h"

#define FILE_NOT_MODIFIED Y("The file has not been modified.")

extern std::unique_ptr<mtx::doc_type_version_handler_c> g_doc_type_version_handler;
