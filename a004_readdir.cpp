#include <iostream>
#include <fstream>
#include <cstdio>
#include <dirent.h>
#include <unistd.h>

using std::cin;
using std::cout;
using std::endl;


static void dirent_describe(const struct dirent& dirent) {
	cout << "dirent.d_ino: " << dirent.d_ino << ", size: " << sizeof(dirent.d_ino) << endl;
	cout << "dirent.d_reclen: " << dirent.d_reclen << ", size: " << sizeof(dirent.d_reclen) << endl;
	cout << "dirent.d_type: " << (int)(unsigned char)dirent.d_type << ", size: " << sizeof(dirent.d_type) << endl;
	cout << "dirent.d_namlen: " << dirent.d_namlen << ", size: " << sizeof(dirent.d_namlen) << endl;
    std::string name = std::string(dirent.d_name, dirent.d_name + dirent.d_namlen);
//  cout << "dirent.d_name: " << dirent.d_name[__DARWIN_MAXNAMLEN + 1] << endl;
    cout << "dirent.d_name: " << name << endl;
    cout << "__DARWIN_MAXNAMLEN: " << __DARWIN_MAXNAMLEN << endl;
}

static void lmi_getline(std::istream& istream, std::string& line);

template <typename T, typename U>
struct Pair {
    T command;
    U method;
};

class Program {
private:
    std::string dirPath;
    std::string inputPath;
    std::string outputPath;

    DIR* dir;
    std::ifstream input;
    std::ofstream output;

    typedef void* (Program::*Method)(void);
    typedef Pair<const char*, Method> MethodPair;

    static void describeMethodCommand(void);

    // describe
    void* describe(void);
    void* describeDIR(void);
    void* describedirent(void);
    void* describeInputOpen(void);
    void* describeOutputOpen(void);

    // isOpen
    bool isInputOpen(void);
    bool isOutputOpen(void);

    // open
    void* openDIR(void);
    void* openInput(void);
    void* openOutput(void);

    // close
    void* close(void);
    void* closeInput(void);
    void* closeOutput(void);

    // unlink
    void* unlink(void);

    // manipulate file
    void* read(void);
    void* write(void);

    // describe
    void* custom(void);

    // describe
    static const MethodPair methodDictionary[];

public:
    void mainLoop(void);
};

const Program::MethodPair Program::methodDictionary[] = {
    { "describe", &Program::describe },
    { "describe dirent", &Program::describedirent },
    { "open input", &Program::openInput },
    { "open output", &Program::openOutput },
    { "close", &Program::close },
    { "close input", &Program::closeInput },
    { "close output", &Program::closeOutput },
    { "unlink", &Program::unlink },
    { "read", &Program::read },
    { "write", &Program::write },
    { "custom", &Program::custom },
};



// MARK: - method
void* Program::describe(void) {
    cout << "dir path: " << this->dirPath << endl;
    cout << "input path: " << this->inputPath << endl;
    cout << "output path: " << this->outputPath << endl;

    this->describeDIR();
    this->describeInputOpen();
    this->describeOutputOpen();

    return NULL;
}

void* Program::describeInputOpen(void) {
    if (this->isInputOpen())
        cout << "yes! input file is open" << endl;
    else
        cout << "no! input file is closed" << endl;

    return NULL;
}

void* Program::describeOutputOpen(void) {
    if (this->isOutputOpen())
        cout << "yes! output file is open" << endl;
    else
        cout << "no! output file is closed" << endl;

    return NULL;
}

void* Program::describeDIR(void) {
    if (this->dir != NULL)
        cout << "DIR*: " << this->dir << endl;
    else
        cout << "dir is NULL" << endl;

    return NULL;
}

void* Program::describedirent(void) {
    this->openDIR();

    while (true) {
        const struct dirent* entry = readdir(this->dir);
        if (entry == NULL)
            break;
        dirent_describe(*entry);
    }

    closedir(this->dir);
    this->dir = NULL;
    this->dirPath.clear();

    return NULL;
}

bool Program::isInputOpen(void) {
    return this->input.is_open();
}

bool Program::isOutputOpen(void) {
    return this->output.is_open();
}

void* Program::openDIR(void) {
    if (this->dir != NULL)
        throw "dir is already open";

    cout << "enter new dir path: ";
    lmi_getline(cin, this->dirPath);
    this->dir = opendir(this->dirPath.c_str());
    if (this->dir != NULL)
        cout << "succeeded opendir()" << endl;
    else {
        this->dirPath.clear();
        throw "failed opendir()";
    }

    return NULL;
}

void* Program::openInput(void) {
    if (this->isInputOpen())
        throw "input is already  open!";

    cout << "current input path: " << this->inputPath << endl;
    cout << "enter new input file path: ";

    lmi_getline(cin, this->inputPath);
    this->input.open(this->inputPath);
    if (this->input.is_open())
        cout << "succeeded opening: " << this->inputPath << endl;
    else {
        this->inputPath.clear();
        cout << "failed opening: " << this->inputPath << endl;
    }

    return NULL;
}

void* Program::openOutput(void) {
    if (this->isOutputOpen())
        throw "output is already  open!";

    cout << "current output path: " << this->outputPath << endl;
    cout << "enter new output file path: ";

    lmi_getline(cin, this->outputPath);
    this->output.open(this->outputPath);
    if (this->output.is_open())
        cout << "succeeded opening: " << this->outputPath << endl;
    else {
        this->outputPath.clear();
        cout << "failed opening: " << this->outputPath << endl;
    }

    return NULL;
}

void* Program::close(void) {
    this->closeInput();
    this->closeOutput();

    return NULL;
}

void* Program::closeInput(void) {
    if (!this->isInputOpen())
        throw "no file to close";

    this->inputPath.clear();
    this->input.close();

    cout << "input is closed" << endl;

    return NULL;
}

void* Program::closeOutput(void) {
    if (!this->isOutputOpen())
        throw "no file to close";

    this->outputPath.clear();
    this->output.close();

    cout << "output is closed" << endl;

    return NULL;
}

void* Program::unlink(void) {
    cout << "enter file path to unlink: ";

    std::string path;
    lmi_getline(cin, path);
    if (::unlink(path.c_str()) == 0)
        cout << "unlinked " << path << endl;
    else
        throw "no that path";

    return NULL;
}

void* Program::custom(void) {
    if (this->isOutputOpen())
        throw "output is already  open!";

    cout << "current output path: " << this->outputPath << endl;
    cout << "enter new output file path: ";

    lmi_getline(cin, this->outputPath);
    this->output.open(this->outputPath, std::ios_base::out);
    if (this->output.is_open())
        cout << "succeeded opening: " << this->outputPath << endl;
    else
        cout << "failed opening: " << this->outputPath << endl;

    return NULL;
}

void* Program::read(void) {
    if (!this->isInputOpen())
        throw "no file to read";

    std::string line;
    lmi_getline(this->input, line);
    cout << "line: " << line << endl;

    return NULL;
}

void* Program::write(void) {
    if (!this->isOutputOpen())
        throw "no file to write";

    std::string line;
    lmi_getline(cin, line);
    this->output << line << endl;
    cout << "write succeeded" << endl;

    return NULL;
}



// MARK: - static
void Program::mainLoop(void) {
    std::string line;

    while (true) {
        cout << endl << "enable command list:" << endl;
        Program::describeMethodCommand();
        cout << "enter command: ";

        lmi_getline(cin, line);
        cout << "----------" << endl;

        unsigned long i;
        for (i = 0; i < sizeof(methodDictionary) / sizeof(MethodPair); ++i) {
            const MethodPair* pair = &methodDictionary[i];

            if (line == pair->command)
                break;
        }
        if (i == sizeof(methodDictionary) / sizeof(MethodPair)) {
            cout << "not found: [" << line << "]" << endl;
            continue;
        }
        try {
            const MethodPair* pair = &methodDictionary[i];
            (this->*(pair->method))();
        } catch(const char* string) {
            cout << line << ": " << string << endl;
        }
    }
}

void Program::describeMethodCommand(void) {
    for (unsigned long i = 0; i < sizeof(methodDictionary) / sizeof(MethodPair); ++i) {
        const MethodPair& pair = methodDictionary[i];

        cout << "\t" << pair.command << endl;
    }
}

static void lmi_getline(std::istream& istream, std::string& line) {
    std::getline(istream, line);
    if (!istream)
        throw "failed getline";
}



// MARK: - main
int main(void) {
    Program program;

    program.mainLoop();

    return 0;
}
