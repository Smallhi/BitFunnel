// Copyright 2011 Microsoft Corporation. All Rights Reserved.
// Author: mhop@microsoft.com (Mike Hopcroft)

#pragma once

#include <istream>
#include <ostream>
#include <string>

namespace BitFunnel
{
    class FileHeaderData;
    class FileHeaderTime;
    class Version;

    //*************************************************************************
    //
    // FileHeader
    //
    // Represents the header portion of a data file.
    //
    // File headers are UTF-8 and designed to be human readable and future
    // proof. Future proofing means that any version of FileHeader can 
    // successfully recover the version number from any future version and
    // determine whether it is able to parse the rest of the header.
    //
    // The first line in the header consists of  
    //   three-part version number
    //   a single space
    //   timestamp
    //   a newline character
    //
    // The remainder of the header consists of
    //   zero or more characters of user data
    //   a single UTF-8 zero
    //
    // The version number consists of three non-negative integers, separated
    // by periods:
    //   [0-9]*.[0-9]*.[0-9]*
    // Version each component of the version number must fit into an unsigned
    // int.
    //
    // The timestamp is in GMT and has the following form:
    //   mm/dd/yyyy hh:mm:ss
    // where m is in (00..11), dd is in (01..31), yyyy is in (1900..9999) and 
    // hh is in (00..23) and mm is in (00..59) and ss is in (00.59).
    //
    // The user data may contain any printable characters and must be terminated
    // by a UTF-8 zero.
    //
    // Here are some examples of legal headers:
    //      "1.23.4 04/20/2011 02:24:45\nHere is some user data."
    //      "1.23.4 04/20/2011 02:24:45\nHere is some user data.\nHere is some more user data."
    //      "1.23.4 04/20/2011 02:24:45\n"
    //*************************************************************************
    class FileHeader
    {
    public:
        // Build-time constructor.
        FileHeader(const Version& version, const std::string& userData);

        // Creates a FileHeader object by parsing the data at the current stream position.
        // If successful, the stream will be positioned on the first byte after the header.
        // In the event of an error, an exception will be thrown and the stream position
        // will be undefined.
        FileHeader(std::istream& in);

        ~FileHeader();

        // Generates a timestamp and writes the header to a stream.
        void Write(std::ostream& out) const;

        // Returns the header's version field.
        const Version& GetVersion() const;

        // Returns the header's timestamp field.
        const time_t& TimeStamp() const;

        // Returns the header's user data field. 
        const std::string& UserData() const;

    private:
        // Initialize by reading from input stream.
        void Read(std::istream& in);
        Version* m_version;
        FileHeaderTime* m_time;
        FileHeaderData* m_userData;
    };

    // A helper class to handle the time stamp in FileHeader.
    class FileHeaderTime
    {
    public:
        FileHeaderTime();
        FileHeaderTime(std::istream& in);
        void Write(std::ostream& out) const;
        const time_t& TimeStamp() const;

    private:
        time_t m_time;
    };

    // A helper class to handle the user data in FileHeader.
    class FileHeaderData
    {
    public:
        static const int c_maxUserDataLen = 1024 * 1024;
        FileHeaderData(const std::string& userData);
        FileHeaderData(std::istream& in);
        void Write(std::ostream& out) const;
        const std::string& Data() const;
    private:
        std::string m_data;
    };
}