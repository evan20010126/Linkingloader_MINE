#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>
#include <bitset>
#include <cmath>

using namespace std;

// global variables //
map<string, int> ESTAB;
char *analog_memory; // 模擬的記憶體

// function declare //
int pass1_execute(int argc, const char *argv[]);
int String_to_Int(string input);
vector<string> read_objfile(const char filename[]);
string getNameWithoutSpace(string originalname);

void pass2_execute(int argc, const char *argv[]);
void processTRecord(string line, int CSADDR, int PROGADDR);
void processMRecord(string line, int CSADDR, int PROGADDR);
int getDec(string input, int bits);
string gethex_string(int input, int bits);

// main //
int main(int argc, const char *argv[])
{
    if (argc < 3)
    {
        cout << "error: 外部參數至少要有三個\n";
        throw;
    }
    int memoryspace = pass1_execute(argc, argv);
    for (auto &item : ESTAB)
    {
        cout << item.first << ": ";
        cout << hex << item.second << endl;
    }

    /* 宣告模擬的記憶體 */
    // memoryspace * 2 的原因: 要兩個16進位符號才代表一個byte
    analog_memory = new char[memoryspace * 2];
    for (int i = 0; i < memoryspace * 2; i++)
        analog_memory[i] = '.';

    pass2_execute(argc, argv);

    int PROGADDR = String_to_Int(argv[1]);

    for (int i = 0; i < memoryspace * 2; ++i)
    {
        if (i % 32 == 0)
        {
            stringstream ss;
            string line_ADDR;
            ss << hex << PROGADDR + (int)(i / 2);
            ss >> line_ADDR;
            transform(line_ADDR.begin(), line_ADDR.end(), line_ADDR.begin(), ::toupper);

            /* 不足 4 bits 的話補0 */
            int fill_num = 4 - line_ADDR.size();
            string fill_str;
            for (int k = 0; k < fill_num; ++k)
            {
                fill_str.push_back('0');
            }
            line_ADDR = fill_str + line_ADDR;
            cout << endl
                 << line_ADDR;
        }
        cout << " " << analog_memory[i];
    }
}

// function define //
int pass1_execute(int argc, const char *argv[])
{
    int PROGADDR = String_to_Int(argv[1]);
    // 將16進制的String轉換成int

    int CSADDR = PROGADDR;
    int memoryspace = 0;
    // 記憶體是用模擬的

    for (int i = 2; i < argc; ++i)
    {
        /* 讀檔案 */
        vector<string> lines = read_objfile(argv[i]);

        string csname = getNameWithoutSpace(lines[0].substr(1, 7));
        int cslength = String_to_Int(lines[0].substr(13, 7)); // CSLTH
        memoryspace += cslength;
        // 算出記憶體要多大的空間

        /* enter control section name into ESTAB */
        ESTAB.insert(pair<string, int>(csname, CSADDR));
        // Ex. 當 PROGA JUMP 到 PROGB 時會用到

        /* 處理所有D record的行 */
        for (int line_num = 1; line_num < lines.size(); ++line_num)
        {
            if (lines[line_num].substr(0, 1) != "D")
                continue;

            int total_round = (lines[line_num].size() - 1) / 12;
            // 算總共有幾個 D record 的 label
            // 減掉1，是因為要把'D'去掉
            // C++ 讀檔這裡不會讀到'\n'，不用跟老師一樣-2

            // cout << lines[line_num].size() << endl;
            for (int item_num = 0; item_num < total_round; ++item_num)
            {
                int start_index;
                start_index = 1 + (12 * item_num);
                string label = getNameWithoutSpace(lines[line_num].substr(start_index, 6));

                start_index += 6;
                int addr = String_to_Int(lines[line_num].substr(start_index, 6));

                ESTAB[label] = CSADDR + addr;
            }
        }
        CSADDR += cslength;
        // 使CSADDR變成下一個Control section的起始位址
    }
    return memoryspace;
}

int String_to_Int(string input)
{
    stringstream ss;
    int output;
    ss << hex << input;
    ss >> output;
    return output;
}

vector<string> read_objfile(const char filename[])
{
    vector<string> lines;
    ifstream ifs(filename, ios::in);
    if (!ifs.is_open())
        cout << "Failed to open file.\n";
    else
    {
        string s;
        while (getline(ifs, s))
            lines.push_back(s);
        ifs.close();
    }
    return lines;
}

string getNameWithoutSpace(string originalname)
{
    // 下面的function好像去不掉空格
    // remove(originalname.begin(), originalname.end(), ' ');
    string output;
    for (int i = 0; i < originalname.size(); i++)
    {
        if (originalname[i] == ' ')
            break;
        output.push_back(originalname[i]);
    }
    return output;
}

void pass2_execute(int argc, const char *argv[])
{
    int PROGADDR = String_to_Int(argv[1]);
    // 將16進制的String轉換成int

    int CSADDR = PROGADDR;
    for (int i = 2; i < argc; ++i)
    { // 讀各個 obj file 分別處理 T record, M record
        /* 讀檔案 */
        vector<string> lines = read_objfile(argv[i]);
        int cslength = String_to_Int(lines[0].substr(13, 7)); // CSLTH

        for (int line_num = 1; line_num < lines.size(); ++line_num)
        {
            string line_type = lines[line_num].substr(0, 1);
            if (line_type == "T")
            {
                processTRecord(lines[line_num], CSADDR, PROGADDR);
            }
            else if (line_type == "M")
            {
                processMRecord(lines[line_num], CSADDR, PROGADDR);
            }
        }
        CSADDR += cslength;
        // 把 CSADDR 變成下一個obj file載入的頭的address
    }
}

void processTRecord(string line, int CSADDR, int PROGADDR)
{
    // line 為一行 T record

    int addr = String_to_Int(line.substr(1, 6));
    // 此行起始的 address
    // 1~7為16進位(0x)，16進位表示法轉換成int

    addr += CSADDR; // Ex. 000054 先加上 PROGA 的位址

    addr -= PROGADDR;
    // 減掉程式最一開始放的 address
    // 轉換成模擬記憶體的陣列索引(analog_memory index是由0開始的陣列)

    addr *= 2; // 因為每一個byte需要兩個element來放，算出來的 index 要乘以2

    int len = String_to_Int(line.substr(7, 2));
    // 此行 T record紀錄 instruction 的長度

    for (int i = 0; i < len * 2; ++i) // ++addr 代表逐個填進去
    {
        analog_memory[addr + i] = line[9 + i];
        // 從第九個位子開始一個一個加進去memory內(copy paste)
    }
}

void processMRecord(string line, int CSADDR, int PROGADDR)
{
    // line 為一行 T record
    int addr = String_to_Int(line.substr(1, 6));
    // 此要修改的 address
    // 1~7為16進位(0x)，16進位表示法轉換成int

    addr += CSADDR; // Ex. 000054 先加上 PROGA 的位址
    cout << "Target address (include opcode): ";
    cout << hex << addr << endl;

    addr -= PROGADDR;
    // 減掉程式最一開始放的 address
    // 轉換成模擬記憶體的陣列索引(analog_memory index是由0開始的陣列)

    addr *= 2; // 因為每一個byte需要兩個element來放，算出來的 index 要乘以2

    int len = String_to_Int(line.substr(7, 2));
    // 此行 M record 紀錄要修改的長度

    if (len == 5)  // 當是05時
        addr += 1; // 因為還有xbpe，加一個半byte，表示改後面五個

    /* 取出在memorycontent中addr ~addr + 長度的index的值，並轉為int*/
    string content;
    for (int i = 0; i < len; i++)
    {
        content.push_back(analog_memory[addr + i]);
    }

    cout << "original content:" << content << endl;
    int value = getDec(content, len);
    string token = line.substr(10, line.size() - 10);
    // line.size()不會多取到'\n'長度，不用多-1
    // C++ 讀檔不會讀到'\n'，不用跟老師一樣-1
    /* 測試
        cout << endl << line << endl;
        cout << dec << line.size() << endl;
        cout << "|" << token << "|";
    */

    if (line.substr(9, 1) == "+")
    {
        value += ESTAB[token]; // ESTAB 為全域變數，在 pass1 已經maintain 好
    }
    else
    {
        value -= ESTAB[token];
    }

    /* 加和減的結果轉換成16進制的string表示 */
    string result = gethex_string(value, len);
    transform(result.begin(), result.end(), result.begin(), ::toupper);

    cout << "modifiy content:" << result << endl
         << endl;
    /* 把算完的結果填回去虛擬memory內 */
    for (int i = 0; i < len; i++)
    {
        analog_memory[addr + i] = result[i];
    }
}

int getDec(string input, int bits)
{
    // usage:
    // 用字串表示的"16進制符號"轉換成integer並考慮為負數的情況(二補數)
    int output = 0; // 要輸出的整數
    bool isNegative;

    if (bits == 5)
    {
        bitset<5 * 4> b; // 放全部變2進制的數字

        int put_into_index = 0; // 0 ~ 19
        for (int i = 4; i > -1; --i)
        { // init b
            string char_to_string(1, input[i]);
            // 第一次拿字串最右邊的字母 Ex. 1111F 的 'F'

            int word = String_to_Int(char_to_string);
            bitset<4> temp(word);

            for (int j = 0; j < 4; ++j)
            {
                if (temp[j] == 1)
                {
                    b.set(put_into_index);
                }
                else
                {
                    b.reset(put_into_index);
                }
                ++put_into_index;
            }
        }

        isNegative = (b[19] == 1) ? true : false;

        if (isNegative)
        {
            /* 定位最右邊的1 */
            int index;
            for (index = 0; index < 20; ++index)
            {
                if (b[index] == 1)
                    break;
            }

            for (index = index + 1; index < 20; ++index)
            {
                b.flip(index);
            }
            output = -b.to_ulong(); // 換成10進制且加上負號
            return output;
        }
        else
        {
            output = String_to_Int(input);
        }
        return output;
    }
    else if (bits == 6)
    {
        bitset<6 * 4> b; // 放全部變2進制的數字

        int put_into_index = 0; // 0 ~ 23
        for (int i = 5; i > -1; --i)
        { // init b
            string char_to_string(1, input[i]);
            // 第一次拿字串最右邊的字母 Ex. 11111F 的 'F'

            int word = String_to_Int(char_to_string);
            bitset<4> temp(word);

            for (int j = 0; j < 4; ++j)
            {
                if (temp[j] == 1)
                {
                    b.set(put_into_index);
                }
                else
                {
                    b.reset(put_into_index);
                }
                ++put_into_index;
            }
        }

        isNegative = (b[23] == 1) ? true : false;

        if (isNegative)
        {
            /* 定位最右邊的1 */
            int index;
            for (index = 0; index < 24; ++index)
            {
                if (b[index] == 1)
                    break;
            }

            for (index = index + 1; index < 24; ++index)
            {
                b.flip(index);
            }
            output = -b.to_ulong(); // 換成10進制且加上負號
            return output;
        }
        else
        {
            output = String_to_Int(input);
        }
        return output;
    }
}

string gethex_string(int input, int bits)
{
    // 將int轉換成有 5bits 或 6bits 的16進制表示的字串

    if (input < int(-pow(2, bits * 4 - 1)) && input > int(pow(2, bits * 4 - 1)) - 1)
    { // 表使超過可表示的bits數了 -> error
        cout << int(-pow(2, bits * 4 - 1)) << endl;
        cout << int(pow(2, bits * 4 - 1)) - 1 << endl;
        cout << "error: bits數不夠表示\n"
             << "input num:" << input << "\nbits:" << bits << "\n";
        throw;
    }

    string output;
    string suffix;
    if (input >= 0)
    { // 大於等於0
        stringstream ss;
        ss << hex << input;
        ss >> suffix;

        if (suffix.size() <= bits)
        {
            int fill_num = bits - suffix.size();

            /* 把前面的bits(0)補齊 */
            for (int i = 0; i < fill_num; ++i)
                output.push_back('0');
            for (auto i : suffix)
                output.push_back(i);
        }
    }
    else
    { //小於0
        /* 想法:
        -10 ->
         10 ->
         00000000000000001010(bitset 20bits) ->
         11111111111111110110 ->
         1048566(轉成int) ->
         FFFF6 (用stringstream 轉十六進制的字串)
        */
        int num = -input;
        if (bits == 5)
        {
            bitset<5 * 4> b(num); // index編碼方式: 19 ... 2 1 0
            int index;
            /* 找最右邊的1的index */
            for (index = 0; index < 20; ++index)
            {
                if (b[index] == 1)
                    break;
            }
            for (index = index + 1; index < 20; ++index)
            {
                b.flip(index);
            }
            int number = b.to_ullong();
            stringstream ss;
            ss << hex << number;
            ss >> output;
        }
        else if (bits == 6)
        {
            bitset<6 * 4> b(num); // index編碼方式: 23 ... 2 1 0
            int index;
            /* 找最右邊的1的index */
            for (index = 0; index < 24; ++index)
            {
                if (b[index] == 1)
                    break;
            }
            for (index = index + 1; index < 24; ++index)
            {
                b.flip(index);
            }
            int number = b.to_ullong();
            stringstream ss;
            ss << hex << number;
            ss >> output;
        }
    }
    return output;
}
/// --end-- //