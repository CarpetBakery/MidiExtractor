#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

// * Header macros
#define MAGIC_HEADER "MThd"
#define MAGIC_TRACK "MTrk"

// * For convenience :(
using namespace std;

// File reading helpers
template<unsigned len> string readString(istream& stream)
{
    char buff[len + 1] = {0};
    stream.read(buff, len);
    return std::string(buff);
}

template <typename T, unsigned length=sizeof(T)> T readBytes(std::ifstream& f)
{
	T temp;
	f.read((char*)&temp, length);
	return temp;
}

void readBytesToFile(ifstream& f, ofstream& oFile, size_t len)
{
    for (int i=0; i<len; i++)
        oFile << (unsigned char)f.get();
}


void writeMidiFile(ifstream& f, size_t offset)
{
    // Output file stream
    ofstream oFile;

    // Ouput file name
    string oFileName = "output/Midi" + to_string((int)offset - 4) + ".mid";

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
        chunkSize = __builtin_bswap32(chunkSize);

        // Read track chunk
        readBytesToFile(f, oFile, chunkSize);
    }

    oFile.close();

    cout << "Wrote to midi file: '" << oFileName << "'" << endl << endl;
}

void readMidiFiles(ifstream &f)
{
    while (!f.eof())
    {
        // Char at current offset
        char c = f.get();

        if (c != 'M')
            continue;
        
        // Read rest of magic
        if (c + readString<3>(f) == MAGIC_HEADER)
        {
            streampos offset = f.tellg();
            // cout << "FOUND MIDI FILE AT OFFSET: " << to_string((int)offset - 4) << endl;
            writeMidiFile(f, static_cast<int>(offset));
        }
    }
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        cout << "Usage: [filepath]" << endl;
        return 0;
    }

    // For execution time
    auto tStart = chrono::high_resolution_clock::now();

    // Asset file to search for midis
    string fileName = argv[1];

    // Open assets file
    ifstream f;
    f.open(fileName, ios::binary);

    if (f.is_open())
    {
        // Try to read midi data from file
        readMidiFiles(f);
        // Close file
        f.close();
    }
    else
    {
        cout << fileName << " does not exist" << endl;
    }

    // Get time elapsed
    auto tEnd = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(tEnd - tStart);
    cout << "Time taken: " << duration.count() << "ms" << endl;

    cout << "end of program" << endl;
    return 0;
}