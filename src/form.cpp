#include <api.h>

#include <StaticHook.h>
#include <algorithm>
#include <functional>
#include <hook.h>
#include <log.h>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <minecraft/net/NetworkIdentifier.h>

#include "base.h"

using JSON   = json::JSON;
using JCLASS = json::JSON::Class;

struct Jsonable {
  virtual JSON toJSON() const = 0;
};

struct BaseForm : Jsonable {
  std::string title;
  BaseForm(std::string title)
      : title(title) {}
  virtual ~BaseForm() {}
  virtual std::string type() const = 0;
  JSON toJSON() const {
    JSON json;
    json["title"] = JSON(title);
    json["type"]  = JSON(type());
    return json;
  }
};

struct ModalForm : BaseForm {
  std::string content, button1, button2;
  ModalForm(std::string title, std::string content, std::string button1, std::string button2)
      : BaseForm(title)
      , content(content)
      , button1(button1)
      , button2(button2) {}
  std::string type() const { return "modal"; }
  JSON toJSON() const {
    auto json       = BaseForm::toJSON();
    json["content"] = JSON("content");
    json["button1"] = JSON("button1");
    json["button2"] = JSON("button2");
    return json;
  }
};

struct BaseControl : Jsonable {
  std::string text;
  BaseControl(std::string text)
      : text(text) {}
  virtual ~BaseControl() {}
  virtual std::string type() const = 0;
  virtual JSON toJSON() const {
    JSON ret;

    ret["text"] = JSON(text);
    ret["type"] = JSON(type());
    return ret;
  }
};

struct LabelControl : BaseControl {
  LabelControl(std::string text)
      : BaseControl(text) {}
  std::string type() const { return "label"; }
};

struct InputControl : BaseControl {
  std::string placeholder, defaultValue;
  InputControl(std::string text, std::string placeholder, std::string defaultValue)
      : BaseControl(text)
      , placeholder(placeholder)
      , defaultValue(defaultValue) {}

  std::string type() const { return "input"; }
  JSON toJSON() const {
    JSON ret = BaseControl::toJSON();

    ret["placeholder"]  = JSON(placeholder);
    ret["defaultValue"] = JSON(defaultValue);
    return ret;
  }
};

struct ToggleControl : BaseControl {
  bool defaultValue;
  ToggleControl(std::string text, bool defaultValue)
      : BaseControl(text)
      , defaultValue(defaultValue) {}

  std::string type() const { return "toggle"; }
  JSON toJSON() const {
    JSON ret = BaseControl::toJSON();

    ret["defaultValue"] = JSON(defaultValue);
    return ret;
  }
};

struct SliderControl : BaseControl {
  float min, max, step, defaultValue;
  SliderControl(std::string text, float min, float max, float step, float defaultValue)
      : BaseControl(text)
      , min(min)
      , max(max)
      , step(step)
      , defaultValue(defaultValue) {}

  std::string type() const { return "slider"; }
  JSON toJSON() const {
    JSON ret = BaseControl::toJSON();

    ret["min"]          = JSON(min);
    ret["max"]          = JSON(max);
    ret["step"]         = JSON(step);
    ret["defaultValue"] = JSON(defaultValue);
    return ret;
  }
};

struct StepsControl : BaseControl {
  std::vector<std::string> steps;
  int defaultValue;
  StepsControl(std::string text, int defaultValue)
      : BaseControl(text)
      , defaultValue(defaultValue) {}
  void add(std::string step) { steps.push_back(step); }
  std::string type() const { return "step_slider"; }
  JSON toJSON() const {
    JSON ret = BaseControl::toJSON();

    int index = 0;
    for (auto step : steps) ret["steps"][index++] = JSON(step);
    ret["defaultValue"] = JSON(defaultValue);
    return ret;
  }
};
struct DropdownControl : BaseControl {
  std::vector<std::string> options;
  int defaultValue;
  DropdownControl(std::string text, int defaultValue)
      : BaseControl(text)
      , defaultValue(defaultValue) {}
  void add(std::string option) { options.push_back(option); }
  std::string type() const { return "dropdown"; }
  JSON toJSON() const {
    JSON ret = BaseControl::toJSON();

    int index = 0;
    for (auto option : options) ret["options"][index++] = JSON(option);
    ret["defaultValue"] = JSON(defaultValue);
    return ret;
  }
};

struct CustomForm : BaseForm {
  CustomForm(std::string title)
      : BaseForm(title) {}
  std::vector<std::shared_ptr<BaseControl>> content;
  void add(std::shared_ptr<BaseControl> ctl) { content.push_back(ctl); }
  std::string type() const { return "custom_form"; }
  JSON toJSON() const {
    JSON ret = BaseForm::toJSON();

    int index = 0;
    for (auto option : content) ret["content"][index++] = option->toJSON();
    return ret;
  }
};

struct SimpleForm : BaseForm {
  std::string content;
  std::vector<std::string> buttons;
  SimpleForm(std::string title, std::string content)
      : BaseForm(title)
      , content(content) {}
  void add(std::string button) { buttons.push_back(button); }
  std::string type() const { return "form"; }
  JSON toJSON() const {
    JSON ret = BaseForm::toJSON();

    ret["content"] = JSON(content);
    int i          = 0;
    for (auto button : buttons) ret["buttons"][i++]["text"] = JSON(button);
    return ret;
  }
};

struct ModalFormRequestPacket : Packet {
  int id;
  std::string data;
  ModalFormRequestPacket(unsigned char playerSubIndex, int id, std::string data)
      : Packet(playerSubIndex)
      , id(id)
      , data(data) {}
  ~ModalFormRequestPacket() {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

struct ModalFormResponsePacket : Packet {
  int id;
  std::string data;
  ModalFormResponsePacket(unsigned char playerSubIndex, int id, std::string data)
      : Packet(playerSubIndex)
      , id(id)
      , data(data) {}

  virtual void *getId() const;
  virtual void *getName() const;
  virtual void *write(BinaryStream &) const;
  virtual void *read(BinaryStream &);
  virtual void *handle(NetworkIdentifier const &, NetEventCallback &) const;
  virtual bool disallowBatching() const;
};

static std::unordered_map<NetworkIdentifier, std::unordered_map<int, std::function<chaiscript::Boxed_Value(std::string)>>> callbacks;

struct ServerNetworkHandler {};

TInstanceHook(void, _ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK23ModalFormResponsePacket, ServerNetworkHandler,
              NetworkIdentifier const &nid, ModalFormResponsePacket &packet) {
  auto it = callbacks.find(nid);
  if (it != callbacks.end()) {
    auto it2 = it->second.find(packet.id);
    if (it2 != it->second.end()) {
      try {
        auto result = it2->second(packet.data);
        if (chaiscript::boxed_cast<bool>(result)) {
          it->second.erase(it2);
          if (it->second.empty()) { callbacks.erase(it); }
        }
      } catch (const std::exception &e) { Log::error("ChaiForm", "Failed to callback: %s", e.what()); }
    }
  }
}

extern "C" void mod_init() {
  chaiscript::ModulePtr m(new chaiscript::Module());
  onPlayerLeft([](ServerPlayer &player) { callbacks.erase(player.getClientId()); });
  m->add(chaiscript::user_type<BaseForm>(), "BaseForm");
  m->add(chaiscript::user_type<ModalForm>(), "ModalForm");
  m->add(chaiscript::base_class<BaseForm, ModalForm>());
  m->add(chaiscript::constructor<ModalForm(std::string title, std::string modal, std::string button1, std::string button2)>(), "ModalForm");
  m->add(chaiscript::user_type<CustomForm>(), "CustomForm");
  m->add(chaiscript::base_class<BaseForm, CustomForm>());
  m->add(chaiscript::constructor<CustomForm(std::string title)>(), "CustomForm");
  m->add(chaiscript::fun(&CustomForm::add), "add");
  m->add(chaiscript::user_type<SimpleForm>(), "SimpleForm");
  m->add(chaiscript::base_class<BaseForm, SimpleForm>());
  m->add(chaiscript::constructor<SimpleForm(std::string title, std::string content)>(), "SimpleForm");
  m->add(chaiscript::fun(&SimpleForm::add), "add");

  m->add(chaiscript::user_type<BaseControl>(), "BaseControl");
  m->add(chaiscript::user_type<LabelControl>(), "LabelControl");
  m->add(chaiscript::base_class<BaseControl, LabelControl>());
  m->add(chaiscript::constructor<LabelControl(std::string text)>(), "LabelControl");
  m->add(chaiscript::user_type<InputControl>(), "InputControl");
  m->add(chaiscript::base_class<BaseControl, InputControl>());
  m->add(chaiscript::constructor<InputControl(std::string text, std::string placeholder, std::string defaultValue)>(), "InputControl");
  m->add(chaiscript::user_type<SliderControl>(), "SliderControl");
  m->add(chaiscript::base_class<BaseControl, SliderControl>());
  m->add(chaiscript::constructor<SliderControl(std::string text, float min, float max, float step, float defaultValue)>(), "SliderControl");
  m->add(chaiscript::user_type<StepsControl>(), "StepsControl");
  m->add(chaiscript::base_class<BaseControl, StepsControl>());
  m->add(chaiscript::constructor<StepsControl(std::string text, int defaultValue)>(), "StepsControl");
  m->add(chaiscript::fun(&StepsControl::add), "add");
  m->add(chaiscript::user_type<DropdownControl>(), "DropdownControl");
  m->add(chaiscript::base_class<BaseControl, DropdownControl>());
  m->add(chaiscript::constructor<DropdownControl(std::string text, int defaultValue)>(), "DropdownControl");
  m->add(chaiscript::fun(&DropdownControl::add), "add");

  m->add(chaiscript::fun([](ServerPlayer &player, int id, BaseForm &form, std::function<chaiscript::Boxed_Value(std::string ret)> callback) {
           auto json = form.toJSON().dump();
           Log::info("Form", "Try to sending: %s", json.c_str());
           ModalFormRequestPacket packet{ player.getClientSubId(), id, json.c_str() };
           player.sendNetworkPacket(packet);
           Log::info("Form", "Sent");
           callbacks[player.getClientId()][id] = callback;
         }),
         "sendForm");
  loadModule(m);
}