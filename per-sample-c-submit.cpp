#include <lo/lo.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::string slurp(const std::string &fileName);
std::string slurp();

int main(int argc, char *argv[]) {
  std::string sourceCode;
  if (argc > 1)
    sourceCode = slurp(argv[1]);
  else
    sourceCode = slurp();

  lo_blob b = lo_blob_new(sourceCode.size(), sourceCode.data());
  lo_address t = lo_address_new(nullptr, "9010");
  if (!lo_send(t, "/code", "b", b)) {
    printf("failed to send packet\n");
    fflush(stdout);
    exit(1);
  }

  return 0;
}

std::string slurp(const std::string &fileName) {
  using namespace std;
  ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

  ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, ios::beg);

  vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  return string(bytes.data(), fileSize);
}

std::string slurp() {
  using namespace std;
  vector<char> cin_str;
  // 64k buffer seems sufficient
  streamsize buffer_sz = 65536;
  vector<char> buffer(buffer_sz);
  cin_str.reserve(buffer_sz);

  auto rdbuf = cin.rdbuf();
  while (auto cnt_char = rdbuf->sgetn(buffer.data(), buffer_sz))
    cin_str.insert(cin_str.end(), buffer.data(), buffer.data() + cnt_char);
  return string(cin_str.data(), cin_str.size());
}

