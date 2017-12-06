// libNTXM - XM Player Library for the Nintendo DS
// Copyright (C) 2005-2007 Tobias Weyand (0xtob)
//                         me@nitrotracker.tobw.net
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _LINEAR_FREQ_TABLE_
#define _LINEAR_FREQ_TABLE_

#include <nds.h>

#define LINEAR_FREQ_TABLE_MIN_NOTE	264
#define LINEAR_FREQ_TABLE_MAX_NOTE	276
#define N_FINETUNE_STEPS		128
#define LINEAR_FREQ_TABLE_SIZE	(LINEAR_FREQ_TABLE_MAX_NOTE*N_FINETUNE_STEPS)

extern const u32 linear_freq_table[LINEAR_FREQ_TABLE_SIZE];

#endif
