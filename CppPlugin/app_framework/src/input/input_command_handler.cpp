// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/cli_args_parser.h>
#include <app_framework/input/input_command_handler.h>
#include <app_framework/ml_macros.h>

#include <iterator>
#include <memory>
#include <sstream>

#define AKEYCODE_COUNT 289  // Currently 288 AKeycodes exist. To Do find a better way to handle

namespace ml {
namespace app_framework {

InputCommandHandler::InputCommandHandler() {}

InputCommandHandler::~InputCommandHandler() {
  Stop();
}

void InputCommandHandler::AddInputHandler(std::shared_ptr<InputHandler> input) {
  input_handlers_.push_back(input);
}

void InputCommandHandler::Add(const std::string &command, const std::string &description, const ExecuteFn &handler_fn) {
  // Overwrite if the key exists.
  if (handler_fn && command.size() > 0) {
    all_commands_[command] = {handler_fn, description};
    commands_with_description_.push_back(std::make_pair(command, description));
  }
}

void InputCommandHandler::SetErrorHandler(const ExecuteFn &handler_fn) {
  if (handler_fn) {
    error_handler_ = {handler_fn, std::string()};
  }
}

void InputCommandHandler::Start() {
  if (input_handlers_.empty()) {
    return;
  }

  for (const auto &input_handler : input_handlers_) {
    auto on_char = [this](InputHandler::EventArgs key_event_args) {
      OnChar(key_event_args);
    };

    auto on_key_up = [this](InputHandler::EventArgs key_event_args) {
      OnKeyUp(key_event_args);
    };

    input_handler->SetEventHandlers(nullptr, on_char, on_key_up);
  }
}

void InputCommandHandler::Stop() {
  if (input_handlers_.empty()) {
    return;
  }

  for (const auto &input_handler : input_handlers_) {
    input_handler->SetEventHandlers(nullptr, nullptr, nullptr);
  }
}

const std::vector<std::pair<std::string, std::string>>& InputCommandHandler::GetAllAvailableCommands() const {
  return commands_with_description_;
}

void InputCommandHandler::OnChar(InputHandler::EventArgs key_event_args) {
#if ML_LUMIN
  if (key_event_args.key_char >= 0x20 && key_event_args.key_char <= 0x7E) {
    std::lock_guard<std::mutex> locker(input_mutex_);
    command_buffer_.push_back(static_cast<char>(key_event_args.key_char));
  }
#else
  (void)key_event_args;
#endif
}

void InputCommandHandler::OnKeyUp(InputHandler::EventArgs key_event_args) {
#if ML_LUMIN
  std::lock_guard<std::mutex> locker(input_mutex_);
  if (command_buffer_.empty() || key_event_args.key_code >= AKEYCODE_COUNT) {
    return;
  }

  uint32_t key_code = key_event_args.key_code;
  if (key_code == AKEYCODE_ENTER) {
    command_queue_.push(command_buffer_);
    command_buffer_.clear();
  } else if (key_code == AKEYCODE_DEL) {
    command_buffer_.pop_back();
  }
#else
  (void)key_event_args;
#endif
}

void InputCommandHandler::PushCommand(const std::string &command) {
  if (command.empty()) {
    return;
  }
  std::lock_guard<std::mutex> locker(input_mutex_);
  command_queue_.push(command);
}

std::queue<std::string> InputCommandHandler::GetCommands() {
  // read and clear the command queue
  std::queue<std::string> command_queue_read_;

  std::lock_guard<std::mutex> locker(input_mutex_);
  if (!command_queue_.empty()) {
    std::swap(command_queue_, command_queue_read_);
  }
  return command_queue_read_;
}

std::string InputCommandHandler::GetCommand() {
  std::string command;
  std::lock_guard<std::mutex> locker(input_mutex_);
  if (!command_queue_.empty()) {
    command = command_queue_.front();
    command_queue_.pop();
  }
  return command;
}

void InputCommandHandler::ProcessCommands() {
  std::queue<std::string> command_queue = GetCommands();
  while (!command_queue.empty()) {
    ProcessCommand(command_queue.front());
    command_queue.pop();
  }
}

void InputCommandHandler::ProcessCommand() {
  std::string command = GetCommand();
  if (!command.empty()) {
    ProcessCommand(command);
  }
}

void InputCommandHandler::ProcessCommand(const std::string &command) {
  ALOGV("Processing the command \"%s\".", command.c_str());

  // Split the command line string by whitespace.
  const std::vector<std::string> args = cli_args_parser::GetArgs(command.c_str());
  if (args.empty()) {
    return;
  }

  InputCommandHandler::Attributes attributes = GetCommandAttributes(args[0]);
  if (attributes.fn) {
    // Invoke handler function.
    attributes.fn(args);
  } else {
    // Command not found, invoke the error handler.
    ALOGI("Command not found, invoking the error handler.");
    if (error_handler_.fn) {
      error_handler_.fn(std::vector<std::string>());
    } else {
      DefaultErrorHandler(args);
    }
  }
}

InputCommandHandler::Attributes InputCommandHandler::GetCommandAttributes(const std::string &cmd) const {
  auto found_entry = all_commands_.find(cmd.c_str());
  if (found_entry != all_commands_.end()) {
    return found_entry->second;
  } else {
    return {nullptr, std::string()};
  }
}

void InputCommandHandler::DefaultErrorHandler(const std::vector<std::string> &commands) {
  // List all available commands with their description.
  ALOGI("Unknown command \"%s\". Available Commands :", commands.front().c_str());
  for (const auto & [cmd, desc] : commands_with_description_) {
    ALOGI("  %s\t\t%s", cmd.c_str(), desc.c_str());
  }
}

}  // namespace app_framework
}  // namespace ml
