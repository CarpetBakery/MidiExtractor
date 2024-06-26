#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

// For creating an output folder...
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING // To silence visual studio warning about filesystem

#if __cplusplus < 201703L // If the version of C++ is less than 17
// It was still in the experimental:: namespace
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

// Header macros
#define MAGIC_HEADER "MThd"
#define MAGIC_TRACK "MTrk"

// For convenience :(
using namespace std;


// File reading helpers
template<unsigned len> string readString(istream& stream)
{
    char buff[len + 1] = { 0 };
    stream.read(buff, len);
    return std::string(buff);
}

template <typename T, unsigned length = sizeof(T)> T readBytes(std::ifstream& f)
{
    T temp;
    f.read((char*)&temp, length);
    return temp;
}

void readBytesToFile(ifstream& f, ofstream& oFile, size_t len)
{
    unsigned char* buff = new unsigned char[len];

    f.read((char*)buff, len);
    oFile.write((char*)buff, len);

    delete[] buff;
}

bool createDirectoryRecursive(std::string const& dirName, std::error_code& err)
{
    err.clear();
    if (!fs::create_directories(dirName, err))
    {
        if (fs::exists(dirName))
        {
            // The folder already exists
            err.clear();
            return false;
        }
        return true;
    }
    return false;
}


void writeMidiFile(ifstream& f, size_t offset)
{
    // NOTE: If we somehow find a midi header at the very end of a file it would probably not be good
    // Output file
    ofstream oFile;
    string oFileName = "output/Midi - " + to_string((int)offset - 4) + ".mid";

    // Open output file
    oFile.open(oFileName, ios::binary);

    // Write magic header
    oFile << MAGIC_HEADER;

    // Write rest of header info
    readBytesToFile(f, oFile, 10);

    // Read tracks 
    while (!f.eof())
    {
        // Read track header
        string test = readString<4>(f);
        if (test != MAGIC_TRACK)
            break;

        // Get chunk size
        auto preRead = f.tellg();
        uint32_t chunkSize = readBytes<uint32_t>(f);
        f.seekg(preRead);

        // Write magic header
        oFile << MAGIC_TRACK;

        // Read chunk size to file
        readBytesToFile(f, oFile, 4);

        // Convert to little endian
        chunkSize = _byteswap_ulong(chunkSize);

        // Read track chunk
        readBytesToFile(f, oFile, chunkSize);
    }

    oFile.close();

    cout << "Wrote to midi file: '" << oFileName << "'" << endl;
}

void readMidiFiles(ifstream& f, int& fileFoundCount)
{
    // Step through file one byte at a time to find a midi file header
    while (!f.eof())
    {
        // Char at current offset
        char c = f.get();
        streampos pBeforeReadString;
        
        if (c != 'M')
            continue;
        
        // We read 3 bytes with "readString" below.
        // If it ends up not being a midi header, step back 3 bytes so we don't accidentally skip over anything
        pBeforeReadString = f.tellg();

        // Read rest of magic
        if (c + readString<3>(f) == MAGIC_HEADER)
        {
            // We found a midi file
            fileFoundCount++;

            streampos offset = f.tellg();
            // cout << "FOUND MIDI FILE AT OFFSET: " << to_string((int)offset - 4) << endl;
            writeMidiFile(f, static_cast<int>(offset));
        }
        else
        {
            // Jump back 3 bytes
            f.seekg(pBeforeReadString);
        }
    }
}

int main(int argc, char** argv)
{
    // If no cmdline args
    if (argc <= 1)
    {
        cout << "Usage: [filepath]" << endl;
        return 0;
    }

    // For execution time
    auto tStart = chrono::high_resolution_clock::now();
    // Number of midi files found
    int fileFoundCount = 0;
    // Asset filepath to search for midis
    string fileName = argv[1];


    // -- Extract midis --

    // Open assets file
    ifstream f;
    f.open(fileName, ios::binary);

    if (f.is_open())
    {
        // Create output directory
        error_code err;
        if (createDirectoryRecursive("output", err))
        {
            cout << "Could not create output directory" << endl;
            f.close();
            return 0;
        }

        // Try to read midi data from file
        readMidiFiles(f, fileFoundCount);

        // Close file
        f.close();
    }
    else
    {
        cout << fileName << " does not exist" << endl;
    }


    // -- Print end message --
    
    // Get time elapsed
    auto tEnd = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(tEnd - tStart);
    cout << endl << "Time taken: " << duration.count() << "ms" << endl;

    cout << "Number of midi files found: " << fileFoundCount << endl;

    cout << endl << "-- Press ENTER to close --" << endl;
    cin.get();
    return 0;
}