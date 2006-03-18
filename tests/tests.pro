#---------------------------------------------------------------------------
# testts.pro
#
# Build configuration file for Zyzzyva tests using qmake.
#
# Copyright 2006 Michael W Thelen <mike@pietdepsi.com>.
#
# This file is part of Zyzzyva.
#
# Zyzzyva is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Zyzzyva is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#---------------------------------------------------------------------------

TEMPLATE = app
TARGET = test_zyzzyva
CONFIG += qt release thread warn_on assistant qtestlib
QT += xml

ROOT = ..
MOC_DIR = $$ROOT/build/moc
OBJECTS_DIR = $$ROOT/build/obj
INCLUDEPATH += $$ROOT/build/moc $$ROOT/src
DEPENDPATH += $$ROOT/build/moc

LIBS = -lzyzzyva -L$$ROOT/lib

# Source files
SOURCES = \
    WordEngineTest.cpp