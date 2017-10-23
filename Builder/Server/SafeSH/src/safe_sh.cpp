#include <ssh_sanitizer.h>
#include <iostream>
#include <system_error>
#include <boost/filesystem.hpp>

int main(int argc, char **argv) {
  int err;
  try {
    builder::SSH_Sanitizer sanitizer(argc, argv);
    err = sanitizer.sanitized_run();

  } catch (const std::system_error &error) {
    std::cerr << "ERROR: " << error.code() << " - " << error.what() << std::endl;
    err = error.code().value();
  } catch (const boost::filesystem::filesystem_error &error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    err = EXIT_FAILURE;
  } catch (const std::exception &error) {
    std::cerr << "ERROR: " << error.what() << std::endl;
    err = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "ERROR: Unknown" << std::endl;
    err = EXIT_FAILURE;
  }

  return err;
}