//============================================================================
//
// This file is part of the Thea project.
//
// This software is covered by the following BSD license, except for portions
// derived from other works which are covered by their respective licenses.
// For full licensing information including reproduction of these external
// licenses, see the file LICENSE.txt provided in the documentation.
//
// Copyright (C) 2013, Siddhartha Chaudhuri/Princeton University
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holders nor the names of contributors
// to this software may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//============================================================================

#include "FileSystem.hpp"
#include "StringAlg.hpp"
#include <boost/filesystem.hpp>
#include <cstdio>

namespace Thea {

bool
FileSystem::exists(std::string const & path)
{
  return boost::filesystem::exists(path);
}

bool
FileSystem::fileExists(std::string const & path)
{
  return boost::filesystem::is_regular_file(path);
}

bool
FileSystem::directoryExists(std::string const & path)
{
  return boost::filesystem::is_directory(path);
}

int64
FileSystem::fileSize(std::string const & path)
{
  uintmax_t size = boost::filesystem::file_size(path);
  if (size == static_cast<uintmax_t>(-1))
    return -1;
  else
    return static_cast<int64>(size);
}

std::string
FileSystem::resolve(std::string const & path)
{
  return boost::filesystem::system_complete(path).string();
}

bool
FileSystem::createDirectory(std::string const & path)
{
  return boost::filesystem::create_directories(path);
}

bool
FileSystem::readWholeFile(std::string const & path, std::string & ret)
{
  if (!fileExists(path))
  {
    THEA_ERROR << "FileSystem: File '" << path << "' not found";
    return false;
  }

  int64 length = fileSize(path);
  if (length <= 0)
  {
    ret.clear();
    return true;
  }

  char * buffer = (char *)std::malloc((size_t)length);
  if (!buffer)
  {
    THEA_ERROR << "FileSystem: Could not allocate buffer to hold " << length << " bytes from file '" << path << '\'';
    return false;
  }

  FILE * f = std::fopen(path.c_str(), "rb");
  if (!f)
  {
    THEA_ERROR << "FileSystem: Couldn't open file '" << path << "' for reading";
    return false;
  }

  int num_read = std::fread(buffer, 1, length, f);
  if (num_read != length)
  {
    THEA_ERROR << "FileSystem: Error reading from file '" << path << '\'';
    return false;
  }

  std::fclose(f);

  ret.assign(buffer, (size_t)length);
  std::free(buffer);

  return true;
}

namespace FileSystemInternal {

bool
entrySatisfiesConstraints(boost::filesystem::directory_entry const & entry, int types, TheaArray<std::string> const & patterns)
{
  if (types > 0 && types != FileSystem::NodeType::ALL)
  {
    boost::filesystem::file_status status = entry.symlink_status();
    if (!boost::filesystem::is_symlink(status))
      status = entry.status();

    bool ok = false;

    if (!ok && (types & FileSystem::NodeType::FILE) && boost::filesystem::is_regular_file(status))
      ok = true;

    if (!ok && (types & FileSystem::NodeType::DIRECTORY) && boost::filesystem::is_directory(status))
      ok = true;

    if (!ok && (types & FileSystem::NodeType::SYMLINK) && boost::filesystem::is_symlink(status))
      ok = true;

    if (!ok)
      return false;
  }

  if (!patterns.empty())
  {
    std::string name = entry.path().filename().string();
    bool ok = false;
    for (array_size_t i = 0; !ok && i < patterns.size(); ++i)
      if (patternMatch(patterns[i], name))
        ok = true;

    if (!ok)
      return false;
  }

  return true;
}

} // namespace FileSystemInternal

long
FileSystem::getDirectoryEntries(std::string const & dir, TheaArray<std::string> & entries, int types,
                                TheaArray<std::string> const & patterns, bool recursive)
{
  if (!directoryExists(dir))
    return -1;

  entries.clear();

  if (recursive)
  {
    boost::filesystem::recursive_directory_iterator entries_end;
    for (boost::filesystem::recursive_directory_iterator iter(dir); iter != entries_end; ++iter)
      if (FileSystemInternal::entrySatisfiesConstraints(*iter, types, patterns))
        entries.push_back(iter->path().string());
  }
  else
  {
    boost::filesystem::directory_iterator entries_end;
    for (boost::filesystem::directory_iterator iter(dir); iter != entries_end; ++iter)
      if (FileSystemInternal::entrySatisfiesConstraints(*iter, types, patterns))
        entries.push_back(iter->path().string());
  }

  return (long)entries.size();
}

} // namespace Thea