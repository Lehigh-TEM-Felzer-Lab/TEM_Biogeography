// tem_yaml.h - YAML reading for XTEM.
//
// Document parses a YAML file once with libyaml and answers key lookups, as the FMS parser module
// does. Reader is what the model uses for tem_config.yaml and the parameter files: typed getters
// by key, and a cursor over the "community" or "temveg" list of a parameter file. Keys may appear
// in any order and may be grouped in sections.
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: YAML reader with strict numeric parsing (replaces ProcessXML and atoi, atof).
// 20241110 Changed: hasEntry and communityCount.
// Reader replaces the ProcessXML base class of every module; its numeric getters are strict.

#ifndef TEM_YAML_H
#define TEM_YAML_H

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tem {
namespace yaml {

struct Node {
  enum Kind { Scalar, Map, Seq };
  Kind kind = Scalar;
  std::string text;                                 // Scalar
  std::vector<std::pair<std::string, Node> > items; // Map, in file order
  std::vector<Node> seq;                            // Seq

  // Direct child of a Map; nullptr if absent or not a Map.
  const Node* child(const std::string& key) const;
};

class Document {
 public:
  Document() {}
  Document(const Document&) = delete;             // index_ points into root_
  Document& operator=(const Document&) = delete;

  // Parse YAML text. label names the source in error messages.
  static std::shared_ptr<Document> parse(const std::string& text,
                                         const std::string& label);
  static std::shared_ptr<Document> load(const std::string& path);

  const Node& root() const { return root_; }
  const std::string& label() const { return label_; }

  // Scalar or Map found under key anywhere outside a sequence (sections may group keys,
  // so "clmstartyr" is found whether it is top level or inside "run:"). Keys must be
  // unique outside sequences; parse() stops with an error otherwise.
  const Node* find(const std::string& key) const;

  // First sequence with this key, for example "community" or "temveg"; nullptr if none.
  const Node* list(const std::string& key) const;

 private:
  Node root_;
  std::string label_;
  std::map<std::string, const Node*> index_;
  void build_index(const Node& n, bool in_seq);
};

// One YAML file of the run: tem_config.yaml or a parameter file.
//
// Scalars are looked up in the open community or temveg entry first and then in the whole file.
// A missing or malformed value stops the run with a message that names the file and the key.
class Reader {
 public:
  // Number of entries in a top-level list of the open file (0 when the list is absent).
  size_t entryCount(const std::string& listkey);
  // Number of 'community' entries. Stops with an error when it exceeds the given maximum.
  int communityCount(int maxcommunities);
  Reader() {}
  explicit Reader(const std::string& path) { open(path); }

  // Parse the file; stops the run if it cannot be read or is not valid YAML.
  void open(const std::string& path);
  // Parse YAML text; label names it in error messages (used by the tests).
  void parse(const std::string& text, const std::string& label);
  bool isOpen() const { return static_cast<bool>(doc_); }
  const std::string& path() const;

  const Node& root() const;
  bool has(const std::string& key) const;
  // True when the current community entry (nextCommunity) has the key
  bool hasEntry(const std::string& key) const;
  // Number of community entries of the file
  size_t communityCount() const;

  bool getBool(const std::string& key) const;
  int getInt(const std::string& key) const;
  long getLong(const std::string& key) const;
  double getDouble(const std::string& key) const;
  std::string getString(const std::string& key) const;
  // The value of a top-level key, or an empty string if the key is absent.
  std::string getOptional(const std::string& key) const;
  // A sequence of scalars, or a single scalar when allow_scalar is set.
  std::vector<std::string> getStringList(const std::string& key, bool allow_scalar = true) const;

  // Advance to the next entry of the "community" or "temveg" list and return its "type".
  int nextCommunity(std::string* description = 0);
  int nextTemveg();
  // Leave the open entry; scalars are then looked up in the whole file again.
  void endEntry() { block_ = 0; }

 private:
  std::shared_ptr<Document> doc_;
  std::string listkey_;
  const Node* list_ = 0;
  size_t next_ = 0;
  const Node* block_ = 0;

  int nextEntry(const std::string& listkey, std::string* description);
  const Node& scalar(const std::string& key) const;
};

}  // namespace yaml
}  // namespace tem

#endif
