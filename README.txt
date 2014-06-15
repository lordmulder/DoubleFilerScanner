Double File Scanner
Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Please check http://muldersoft.com/ for news and updates!


------------------------------------------------------------------------------
1. Introduction
------------------------------------------------------------------------------

The purpose of this tool is scanning the selected directory or directories for
duplicate files, i.e. files with identical content. Duplicate files are
identified by first calculating the SHA-1 digest of each file and then looking
for values that appear more than once. In particular, files with identical
content are guaranteed to have the same SHA-1 digest, while files with
differing content will have different SHA-1 values with very high certainty.

All computed SHA-1 values are stored in a hash table, so collisions are found
quickly and we do NOT need to compare every digest to every other one. Also,
the files are processed concurrently in multiple "worker" threads in order to
parallelize and speed-up the SHA-1 computations on multi-core processors. On
our test machine it took ~15 minutes to analyse all the ~260,000 files on the
system drive (~63.5 GB). During this operation ~44,000 duplicates were found.

The list of identified duplicates can be exported to the XML and INI formats.


------------------------------------------------------------------------------
2. Platform Support
------------------------------------------------------------------------------

The following platforms are supported:
- Windows 8 and 8.1, 32-Bit and 64-Bit versions
- Windows 7, 32-Bit and 64-Bit versions
- Windows XP, Service Pack 3 is required!

Platforms that should work too, but are not tested extensively:
- Windows Vista, 32-Bit and 64-Bit versions
- Windows XP Professional x64 Edition, Service Pack 2 is required!

Download Service Pack 3 for Windows XP here:
http://www.microsoft.com/en-us/download/details.aspx?id=24


------------------------------------------------------------------------------
3. Anti-Virus Warning
------------------------------------------------------------------------------

Anti-Virus programs can interfere with the Double File Scanner software and
significantly slow down the process! Therefore it is highly recommend to turn
off the "real time scanner" or "guard" feature of your Anti-Virus program
while the Double File Scanner is running. But don't forget to re-enable it!


------------------------------------------------------------------------------
4. Command-Line Options
------------------------------------------------------------------------------

The following command-line options are available:

--console            Enable the debug console
--scan <directory>   Scan the specified directory, can be used multiple times


E.O.F.
