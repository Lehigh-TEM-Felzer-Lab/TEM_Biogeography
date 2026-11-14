// tem_yaml.cpp - see tem_yaml.h
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: YAML reader with strict numeric parsing (replaces ProcessXML and atoi, atof).
// 20241110 Changed: hasEntry and communityCount.
// The Reader implementation: strict numeric parsing (strtol, strtod), errors name the file and key.

#include "tem_yaml.h"

#include <yaml.h>

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

namespace tem {
namespace yaml {

namespace {

[[noreturn]] void die(const std::string& msg) {
  std::cerr << std::endl << msg << std::endl;
  std::exit(-1);
}

// ---- libyaml event parser -------------------------------------------------------------

class EventParser {
 public:
  EventParser(const std::string& text, const std::string& label) : label_(label) {
    yaml_parser_initialize(&parser_);
    yaml_parser_set_input_string(
        &parser_, reinterpret_cast<const unsigned char*>(text.data()), text.size());
  }
  ~EventParser() { yaml_parser_delete(&parser_); }

  Node parse_document() {
    yaml_event_t ev;
    next(ev);  // STREAM-START
    yaml_event_delete(&ev);
    next(ev);
    if (ev.type == YAML_STREAM_END_EVENT) {
      yaml_event_delete(&ev);
      Node empty;
      empty.kind = Node::Map;
      return empty;
    }
    if (ev.type != YAML_DOCUMENT_START_EVENT) fail("expected a YAML document");
    yaml_event_delete(&ev);
    next(ev);
    Node root = read_node(ev);
    if (root.kind != Node::Map) fail("the top level of the file must be a mapping");
    return root;
  }

 private:
  yaml_parser_t parser_;
  std::string label_;
  std::map<std::string, Node> anchors_;

  [[noreturn]] void fail(const std::string& what) {
    std::ostringstream os;
    os << "Invalid YAML in " << label_ << ": " << what;
    if (parser_.problem) {
      os << " (" << parser_.problem << " at line " << parser_.problem_mark.line + 1
         << ", column " << parser_.problem_mark.column + 1 << ")";
    }
    die(os.str());
  }

  void next(yaml_event_t& ev) {
    if (!yaml_parser_parse(&parser_, &ev)) fail("syntax error");
  }

  // Consumes ev (and the events of its children) and returns the node it starts.
  Node read_node(yaml_event_t& ev) {
    Node n;
    std::string anchor;
    switch (ev.type) {
      case YAML_SCALAR_EVENT: {
        n.kind = Node::Scalar;
        n.text.assign(reinterpret_cast<char*>(ev.data.scalar.value), ev.data.scalar.length);
        const bool plain = ev.data.scalar.style == YAML_PLAIN_SCALAR_STYLE;
        if (plain && (n.text == "~" || n.text == "null" || n.text == "Null" || n.text == "NULL"))
          n.text.clear();
        if (ev.data.scalar.anchor) anchor = reinterpret_cast<char*>(ev.data.scalar.anchor);
        yaml_event_delete(&ev);
        break;
      }
      case YAML_MAPPING_START_EVENT: {
        n.kind = Node::Map;
        if (ev.data.mapping_start.anchor)
          anchor = reinterpret_cast<char*>(ev.data.mapping_start.anchor);
        yaml_event_delete(&ev);
        for (;;) {
          yaml_event_t kev;
          next(kev);
          if (kev.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&kev);
            break;
          }
          if (kev.type != YAML_SCALAR_EVENT) fail("mapping keys must be plain strings");
          std::string key(reinterpret_cast<char*>(kev.data.scalar.value), kev.data.scalar.length);
          yaml_event_delete(&kev);
          yaml_event_t vev;
          next(vev);
          n.items.push_back(std::make_pair(key, read_node(vev)));
        }
        break;
      }
      case YAML_SEQUENCE_START_EVENT: {
        n.kind = Node::Seq;
        if (ev.data.sequence_start.anchor)
          anchor = reinterpret_cast<char*>(ev.data.sequence_start.anchor);
        yaml_event_delete(&ev);
        for (;;) {
          yaml_event_t sev;
          next(sev);
          if (sev.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&sev);
            break;
          }
          n.seq.push_back(read_node(sev));
        }
        break;
      }
      case YAML_ALIAS_EVENT: {
        const std::string name = reinterpret_cast<char*>(ev.data.alias.anchor);
        yaml_event_delete(&ev);
        std::map<std::string, Node>::const_iterator it = anchors_.find(name);
        if (it == anchors_.end()) fail("alias *" + name + " has no anchor");
        return it->second;
      }
      default:
        fail("unexpected YAML structure");
    }
    if (!anchor.empty()) anchors_[anchor] = n;
    return n;
  }
};

}  // namespace

// ---- Node / Document ------------------------------------------------------------------

const Node* Node::child(const std::string& key) const {
  if (kind != Map) return 0;
  for (size_t i = 0; i < items.size(); ++i)
    if (items[i].first == key) return &items[i].second;
  return 0;
}

std::shared_ptr<Document> Document::parse(const std::string& text, const std::string& label) {
  std::shared_ptr<Document> doc(new Document());
  doc->label_ = label;
  EventParser p(text, label);
  doc->root_ = p.parse_document();
  doc->build_index(doc->root_, false);
  return doc;
}

std::shared_ptr<Document> Document::load(const std::string& path) {
  std::ifstream f(path.c_str());
  if (!f) die("Cannot open " + path + " for YAML input");
  std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return parse(text, path);
}

void Document::build_index(const Node& n, bool in_seq) {
  if (n.kind != Node::Map || in_seq) return;
  for (size_t i = 0; i < n.items.size(); ++i) {
    const std::string& key = n.items[i].first;
    const Node& v = n.items[i].second;
    if (v.kind == Node::Seq) continue;
    if (index_.count(key)) die("Duplicate key '" + key + "' in " + label_ + "; keys must be unique");
    index_[key] = &v;
    build_index(v, false);
  }
}

const Node* Document::find(const std::string& key) const {
  std::map<std::string, const Node*>::const_iterator it = index_.find(key);
  return it == index_.end() ? 0 : it->second;
}

const Node* Document::list(const std::string& key) const {
  const Node* n = root_.child(key);
  if (n && n->kind == Node::Seq) return n;
  return 0;
}

}  // namespace yaml
}  // namespace tem

// ---- Reader ---------------------------------------------------------------------------

namespace tem {
namespace yaml {

namespace {

// YAML booleans are accepted for numeric flags.
std::string numeric(const std::string& t) {
  if (t == "true" || t == "True" || t == "TRUE" || t == "yes" || t == "Yes") return "1";
  if (t == "false" || t == "False" || t == "FALSE" || t == "no" || t == "No") return "0";
  return t;
}

[[noreturn]] void bad_value(const Reader& r, const std::string& key, const std::string& text,
                            const char* want) {
  die("Value '" + text + "' of " + key + " in " + r.path() + " is not " + want);
}

}  // namespace

void Reader::open(const std::string& path) {
  doc_ = Document::load(path);
  listkey_.clear();
  list_ = 0;
  next_ = 0;
  block_ = 0;
}

void Reader::parse(const std::string& text, const std::string& label) {
  doc_ = Document::parse(text, label);
  listkey_.clear();
  list_ = 0;
  next_ = 0;
  block_ = 0;
}

const std::string& Reader::path() const {
  static const std::string none = "(no file)";
  return doc_ ? doc_->label() : none;
}

const Node& Reader::root() const {
  if (!doc_) die("Internal error: a YAML value was requested before a file was opened");
  return doc_->root();
}

bool Reader::has(const std::string& key) const {
  return doc_ && doc_->find(key) != 0;
}

size_t Reader::communityCount() const {
  const Node* l = doc_ ? doc_->list("community") : 0;
  return l ? l->seq.size() : 0;
}

bool Reader::hasEntry(const std::string& key) const {
  return block_ && block_->child(key) != 0;
}

const Node& Reader::scalar(const std::string& key) const {
  if (!doc_) die("Internal error: " + key + " was requested before a YAML file was opened");
  const Node* n = block_ ? block_->child(key) : 0;
  if (!n) n = doc_->find(key);
  if (!n || n->kind != Node::Scalar) die("Cannot find " + key + " in " + doc_->label());
  return *n;
}

std::string Reader::getString(const std::string& key) const {
  return scalar(key).text;
}

std::string Reader::getOptional(const std::string& key) const {
  const Node* n = root().child(key);
  return n ? n->text : std::string();
}

bool Reader::getBool(const std::string& key) const {
  const std::string v = numeric(scalar(key).text);
  if (v != "0" && v != "1") bad_value(*this, key, scalar(key).text, "true or false");
  return v == "1";
}

long Reader::getLong(const std::string& key) const {
  const std::string text = scalar(key).text;
  const std::string v = numeric(text);
  char* end = 0;
  errno = 0;
  const long x = std::strtol(v.c_str(), &end, 10);
  if (end != v.c_str() && *end == '\0' && errno == 0) return x;
  // an integer written as 2.0 or 1e3
  const double d = std::strtod(v.c_str(), &end);
  if (end != v.c_str() && *end == '\0' && std::isfinite(d) && d == std::floor(d) &&
      std::fabs(d) < 9.0e18)
    return static_cast<long>(d);
  bad_value(*this, key, text, "an integer");
}

int Reader::getInt(const std::string& key) const {
  const long x = getLong(key);
  if (x < INT_MIN || x > INT_MAX) bad_value(*this, key, scalar(key).text, "in the range of an integer");
  return static_cast<int>(x);
}

double Reader::getDouble(const std::string& key) const {
  const std::string text = scalar(key).text;
  const std::string v = numeric(text);
  char* end = 0;
  const double x = std::strtod(v.c_str(), &end);
  if (end == v.c_str() || *end != '\0') bad_value(*this, key, text, "a number");
  return x;
}

std::vector<std::string> Reader::getStringList(const std::string& key, bool allow_scalar) const {
  std::vector<std::string> out;
  const Node* s = doc_ ? doc_->list(key) : 0;
  if (s) {
    for (size_t i = 0; i < s->seq.size(); ++i) {
      const Node& item = s->seq[i];
      if (item.kind != Node::Scalar || item.text.empty())
        die("Expected nonempty scalar entries in " + key + " of " + path());
      out.push_back(item.text);
    }
  } else if (allow_scalar) {
    out.push_back(scalar(key).text);
  } else {
    die("Expected a YAML sequence for " + key + " in " + path());
  }
  return out;
}

size_t Reader::entryCount(const std::string& listkey) {
  if (!doc_) die("Internal error: " + listkey + " was counted before a YAML file was opened");
  const Node* l = doc_->list(listkey);
  return l ? l->seq.size() : 0;
}

int Reader::communityCount(int maxcommunities) {
  const size_t n = entryCount("community");
  if (n > static_cast<size_t>(maxcommunities))
    die(path() + " has " + std::to_string(n) + " community entries, more than the " +
        std::to_string(maxcommunities) + " that MAXCMNT allows");
  return static_cast<int>(n);
}

int Reader::nextEntry(const std::string& listkey, std::string* description) {
  if (!doc_) die("Internal error: " + listkey + " was requested before a YAML file was opened");
  if (listkey_ != listkey) {
    listkey_ = listkey;
    list_ = doc_->list(listkey);
    next_ = 0;
  }
  if (!list_ || next_ >= list_->seq.size())
    die("Cannot find a further " + listkey + " entry in " + path());

  block_ = &list_->seq[next_++];
  const Node* type = block_->child("type");
  if (!type) die("A " + listkey + " entry of " + path() + " has no 'type'");
  if (description) {
    const Node* d = block_->child("description");
    *description = d ? d->text : std::string();
  }
  const std::string text = type->text;
  char* end = 0;
  const long x = std::strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0') bad_value(*this, "type", text, "an integer");
  return static_cast<int>(x);
}

int Reader::nextCommunity(std::string* description) {
  return nextEntry("community", description);
}

int Reader::nextTemveg() {
  return nextEntry("temveg", 0);
}

}  // namespace yaml
}  // namespace tem
