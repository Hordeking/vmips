/* Definitions to support the memory module wrapper class.
   Copyright 2001, 2003 Brian R. Gaeke.

This file is part of VMIPS.

VMIPS is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

VMIPS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with VMIPS; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _MEMORYMODULE_H_
#define _MEMORYMODULE_H_

class MemoryModule {
public:
    caddr_t addr;
    uint32 len;
    MemoryModule(size_t size);
    ~MemoryModule();
	void print(void);
};

#endif /* _MEMORYMODULE_H_ */
