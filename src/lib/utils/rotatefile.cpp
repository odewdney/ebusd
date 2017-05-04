/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2016-2017 John Baier <ebusd@ebusd.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/utils/rotatefile.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/ioctl.h>
#include <sys/file.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include "lib/utils/clock.h"

namespace ebusd {

using std::streamsize;

RotateFile::~RotateFile() {
  if (m_stream) {
    fclose(m_stream);
    m_stream = NULL;
  }
}

bool RotateFile::setEnabled(bool enabled) {
  if (enabled == m_enabled) {
    return false;
  }
  m_enabled = enabled;
  if (m_stream) {
    fclose(m_stream);
    m_stream = NULL;
  }
  if (enabled) {
#ifdef _WIN32
	  errno_t e = fopen_s(&m_stream, m_fileName.c_str(), m_textMode ? "w" : "wb");
#else
	  m_stream = fopen(m_fileName.c_str(), m_textMode ? "w" : "wb");
#endif
	  m_fileSize = 0;
  }
  return true;
}

void RotateFile::write(const unsigned char* value, const size_t size, const bool received, const bool bytes) {
  if (!m_enabled || !m_stream) {
    return;
  }
  if (m_textMode) {
#ifdef _WIN32
	  SYSTEMTIME st;
	  GetSystemTime(&st); 
#else
	  struct timespec ts;
    struct tm td;
    clockGettime(&ts);
    localtime_r(&ts.tv_sec, &td);
#endif
	fprintf(m_stream, "%04d-%02d-%02d %02d:%02d:%02d.%03ld ",
#ifdef _WIN32
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
		td.tm_year+1900, td.tm_mon+1, td.tm_mday,
		td.tm_hour, td.tm_min, td.tm_sec, ts.tv_nsec/1000000);
#endif
    if (bytes) {
      fprintf(m_stream, received ? "<" : ">");
      for (unsigned int pos = 0; pos < size; pos++) {
        fprintf(m_stream, "%2.2x ", value[pos]);
      }
    } else {
      fwrite(value, 1, size, m_stream);
    }
    fprintf(m_stream, "\n");
    m_fileSize += 25+3*size+1;
  } else {
    fwrite(value, (streamsize)size, 1, m_stream);
    m_fileSize += size;
  }
  if ((m_fileSize%1024) == 0) {
    fflush(m_stream);
  }
  if (m_fileSize >= m_maxSize * 1024LL) {
    string oldfile = string(m_fileName)+".old";
    if (rename(m_fileName.c_str(), oldfile.c_str()) == 0) {
      fclose(m_stream);
#ifdef _WIN32
	  errno_t e = fopen_s(&m_stream , m_fileName.c_str(), m_textMode ? "w" : "wb");
#else
	  m_stream = fopen(m_fileName.c_str(), m_textMode ? "w" : "wb");
#endif
      m_fileSize = 0;
    }
  }
}

}  // namespace ebusd
