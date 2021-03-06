/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <glog/logging.h>

#include <osquery/core.h>
#include <osquery/tables.h>
#include <osquery/filesystem.h>

namespace osquery {
namespace tables {

void crawl_proc(QueryData& results) {
  std::vector<std::string> processes;

  if (!osquery::procProcesses(processes).ok()) {
    LOG(INFO) << "Cannot list Linux processes";
    return;
  }

  boost::regex socket_filter("[0-9]+");
  for (const auto& process : processes) {
    std::vector<std::string> descriptors;
    if (!osquery::procDescriptors(process, descriptors).ok()) {
      continue;
    }

    for (const auto& descriptor : descriptors) {
      std::string linkname;
      if (!procReadDescriptor(process, descriptor, linkname).ok()) {
        // This is an odd error case, but the symlink could not be read.
        continue;
      }

      if (linkname.find("socket") == std::string::npos) {
        // This is not a socket descriptor.
        continue;
      }

      // The linkname is in the form socket:[12345].
      boost::smatch inode;
      boost::regex_search(linkname, inode, socket_filter);
      if (inode[0].str().length() > 0) {
        Row r;
        r["pid"] = process;
        r["inode"] = inode[0].str();
        results.push_back(r);
      }
    }
  }

  return;
}

QueryData genSocketInode(QueryContext& context) {
  QueryData results;
  crawl_proc(results);
  return results;
}
}
}
