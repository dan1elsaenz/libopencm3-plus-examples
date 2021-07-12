##
## This file is part of the libopencm3-plus project.
##
## Copyright (C) Federico Ruiz Ugalde memeruiz@gmail.com
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = main

TYPE = RECV ## Or TRANS

OOCD_INTERFACE = stlink-v2-1
#OOCD_EXTRA = -c "hla_serial \x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x31"

LDSCRIPT = ../../../../../libopencm3-plus/lib/libopencm3_plus_steval-ids001v4m.ld
LDLIBS +=
CFLAGS += -D$(TYPE)

OBJS	+=

include ../../Makefile.include

dfu: bin
	sudo dfu-util -d 1209:db42 -D $(BINARY).bin
