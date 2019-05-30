#include <vector>

#ifndef JSON_HELPER_HPP_
#define JSON_HELPER_HPP_

String makeJsonString(std::vector<String>& keys, std::vector<String>& vals) {
    String json = "{";
    
    for (int i=0; i<keys.size(); i++) {
      if (i) json += ",";
      json += ("\"" + keys[i] + "\":");
      json += ("\"" + vals[i] + "\"");
    }   
    
    json += "}";

    return json;
}

#endif // JSON_HELPER_HPP_
