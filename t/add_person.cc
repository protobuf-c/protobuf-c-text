// See README.txt for information and build instructions.

#include <iostream>
#include <fstream>
#include <string>
#include "addressbook.pb.h"
using namespace std;

// This function fills in a Person message based on user input.
void PromptForAddress(tutorial::Person* person) {
  cout << "Enter person ID number: ";
  int id;
  cin >> id;
  person->set_id(id);
  cin.ignore(256, '\n');

  cout << "Enter name: ";
  getline(cin, *person->mutable_name());

  cout << "Enter email address (blank for none): ";
  string email;
  getline(cin, email);
  if (!email.empty()) {
    person->set_email(email);
  }

  while (true) {
    cout << "Enter a phone number (or leave blank to finish): ";
    string number;
    getline(cin, number);
    if (number.empty()) {
      break;
    }

    tutorial::Person::PhoneNumber* phone_number = person->add_phone();
    phone_number->set_number(number);

    cout << "Is this a mobile, home, or work phone? ";
    string type;
    getline(cin, type);
    if (type == "mobile") {
      phone_number->set_type(tutorial::Person::MOBILE);
    } else if (type == "home") {
      phone_number->set_type(tutorial::Person::HOME);
    } else if (type == "work") {
      phone_number->set_type(tutorial::Person::WORK);
    } else {
      cout << "Unknown phone type.  Using default." << endl;
    }
  }

  double double_var;
  cout << "Enter double_var (0 to skip): ";
  cin >> double_var;
  if (double_var)
    person->set_double_var(double_var);
  cin.ignore(256, '\n');

  float float_var;
  cout << "Enter float_var (0 to skip): ";
  cin >> float_var;
  if (float_var)
    person->set_float_var(float_var);
  cin.ignore(256, '\n');

  int64_t int64_var;
  cout << "Enter int64_var (0 to skip): ";
  cin >> int64_var;
  if (int64_var)
    person->set_int64_var(int64_var);
  cin.ignore(256, '\n');

  uint32_t uint32_var;
  cout << "Enter uint32_var (0 to skip): ";
  cin >> uint32_var;
  if (uint32_var)
    person->set_uint32_var(uint32_var);
  cin.ignore(256, '\n');

  uint64_t uint64_var;
  cout << "Enter uint64_var (0 to skip): ";
  cin >> uint64_var;
  if (uint64_var)
    person->set_uint64_var(uint64_var);
  cin.ignore(256, '\n');

  int32_t sint32_var;
  cout << "Enter sint32_var (0 to skip): ";
  cin >> sint32_var;
  if (sint32_var)
    person->set_sint32_var(sint32_var);
  cin.ignore(256, '\n');

  int64_t sint64_var;
  cout << "Enter sint64_var (0 to skip): ";
  cin >> sint64_var;
  if (sint64_var)
    person->set_sint64_var(sint64_var);
  cin.ignore(256, '\n');

  uint32_t fixed32_var;
  cout << "Enter fixed32_var (0 to skip): ";
  cin >> fixed32_var;
  if (fixed32_var)
    person->set_fixed32_var(fixed32_var);
  cin.ignore(256, '\n');

  uint64_t fixed64_var;
  cout << "Enter fixed64_var (0 to skip): ";
  cin >> fixed64_var;
  if (fixed64_var)
    person->set_fixed64_var(fixed64_var);
  cin.ignore(256, '\n');

  int32_t sfixed32_var;
  cout << "Enter sfixed32_var (0 to skip): ";
  cin >> sfixed32_var;
  if (sfixed32_var)
    person->set_sfixed32_var(sfixed32_var);
  cin.ignore(256, '\n');

  int64_t sfixed64_var;
  cout << "Enter sfixed64_var (0 to skip): ";
  cin >> sfixed64_var;
  person->set_sfixed64_var(sfixed64_var);
  cin.ignore(256, '\n');

  int bool_var;
  cout << "Enter bool_var (-1 to skip): ";
  cin >> bool_var;
  if (bool_var >= 0) {
    person->set_bool_var(bool_var > 0);
  }
  cin.ignore(256, '\n');

  string string_var;
  cout << "Enter string_var (return to skip): ";
  getline(cin, string_var);
  if (!string_var.empty()) {
    person->set_string_var(string_var);
  }

  string file_name;
  cout << "Enter file_name (0 to skip): ";
  getline(cin, file_name);
  if (!file_name.empty()) {
    ifstream in(file_name.c_str(), ios::binary);
    in.seekg(0, ios::end);
    streamsize length = in.tellg();
    in.seekg(0, ios::beg);

    vector<char> buffer(length);
    if (in.read(buffer.data(), length)) {
      person->set_bytes_var(buffer.data(), length);
    }
  }

}

// Main function:  Reads the entire address book from a file,
//   adds one person based on user input, then writes it back out to the same
//   file.
int main(int argc, char* argv[]) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc != 2) {
    cerr << "Usage:  " << argv[0] << " ADDRESS_BOOK_FILE" << endl;
    return -1;
  }

  tutorial::AddressBook address_book;

  {
    // Read the existing address book.
    fstream input(argv[1], ios::in | ios::binary);
    if (!input) {
      cout << argv[1] << ": File not found.  Creating a new file." << endl;
    } else if (!address_book.ParseFromIstream(&input)) {
      cerr << "Failed to parse address book." << endl;
      return -1;
    }
  }

  // Add an address.
  PromptForAddress(address_book.add_person());

  {
    // Write the new address book back to disk.
    fstream output(argv[1], ios::out | ios::trunc | ios::binary);
    if (!address_book.SerializeToOstream(&output)) {
      cerr << "Failed to write address book." << endl;
      return -1;
    }
  }

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
