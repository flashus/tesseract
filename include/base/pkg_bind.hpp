/* Copyright (C) 2008 Alejandro Álvarez
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

#pragma once
#ifndef TESSERACT_BASE_PKG_BIND_HPP
#define TESSERACT_BASE_PKG_BIND_HPP

#include <memory>
using std::unique_ptr;

#include <QSet>
#include <QString>

#ifdef TESSERACT_USE_VLD
#	include <vld.h>
#endif

class PkgBind: private QObject
{
  Q_OBJECT
 private:
  QString invokeCmd;
  QSet<QString> commands;
  static unique_ptr<PkgBind> instance;

  /* Constructor
   * Singleton
   */
  PkgBind();
 public:
  /* Get the unique instance
   * or create it if there isn't any
   */
  static const unique_ptr<PkgBind> &getInstance();

  /* Load the command list from a file
   */
  void loadCommandList();

  /* Check if a symbol is defined
   * as a function included in some package
   */
  bool checkSymbol(const QString &s);

  /* Invoke the package manager
   * for install the package with the command 
   * "cmd"
   */
  void invokePackageManager(const QString &s);

 public slots:
  void invokeResponse(int result);
};

#endif
