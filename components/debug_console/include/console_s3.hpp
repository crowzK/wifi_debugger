/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef CONSOLE_S3_HPP
#define CONSOLE_S3_HPP

#include "console.hpp"

class ConsoleS3 : protected Console
{
public:
    void help();

protected:
    ConsoleS3();
    ~ConsoleS3() = default;
    
    bool init() override;
    void task() override;
        
    static void cdcRxCallback(int itf, void *event);
    static void cdcLineStateChangedCallback(int itf, void *event);
    static void cdcLineCodingChangedCallback(int itf, void *event);
};

#endif // CONSOLE_S3_HPP
