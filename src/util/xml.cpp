/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <ostream>

#include "string2int.h"
#include "i2string.h"
#include "xml.h"

/*******************************************************************\

Function: xmlt::clear

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::clear()
{
  data.clear();
  name.clear();
  attributes.clear();
  elements.clear();
}

/*******************************************************************\

Function: xmlt::swap

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::swap(xmlt &xml)
{
  xml.data.swap(data);
  xml.attributes.swap(attributes);
  xml.elements.swap(elements);
  xml.name.swap(name);
}

/*******************************************************************\

Function: xmlt::output

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::output(std::ostream &out, unsigned indent) const
{
  // 'name' needs to be set, or we produce mal-formed
  // XML.
  
  if(name=="") return;

  do_indent(out, indent);

  out << '<' << name;

  for(const auto & it : attributes)
  {
    // it.first needs to be non-empty
    if(it.first=="") continue;
    out << ' ' << it.first
        << '=' << '"';
    escape_attribute(it.second, out);
    out << '"';
  }

  if(elements.empty() && data.empty())
  {
    out << "/>" << "\n";
    return;
  }

  out << '>';

  if(elements.empty())
    escape(data, out);
  else
  {
    out << "\n";

    for(const auto & it : elements)
      it.output(out, indent+2);

    do_indent(out, indent);
  }

  out << '<' << '/' << name << '>' << "\n";
}

/*******************************************************************\

Function: xmlt::escape

  Inputs:

 Outputs:

 Purpose: escaping for XML elements

\*******************************************************************/

void xmlt::escape(const std::string &s, std::ostream &out)
{
  for(const auto ch : s)
  {
    switch(ch)
    {
    case '&':
      out << "&amp;";
      break;

    case '<':
      out << "&lt;";
      break;

    case '>':
      out << "&gt;";
      break;
    
    case '\r':
      break; // drop!
    
    case '\n':
      out << '\n';
      break;

    default:
      // &#0; isn't allowed, but what shall we do?
      if((ch>=0 && ch<' ') || ch==127)
        out << "&#"+i2string((unsigned char)ch)+";";
      else
        out << ch;
    }
  }
}

/*******************************************************************\

Function: xmlt::escape_attribute

  Inputs:

 Outputs:

 Purpose: escaping for XML attributes, assuming that
          double quotes " are used consistently,
          not single quotes

\*******************************************************************/

void xmlt::escape_attribute(const std::string &s, std::ostream &out)
{
  for(const auto & ch : s)
  {
    switch(ch)
    {
    case '&':
      out << "&amp;";
      break;

    case '<':
      out << "&lt;";
      break;

    case '>':
      out << "&gt;";
      break;

    case '"':
      out << "&quot;";
      break;

    default:
      // &#0; isn't allowed, but what shall we do?
      if((ch>=0 && ch<' ') || ch==127)
        out << "&#"+i2string((unsigned char)ch)+";";
      else
        out << ch;
    }
  }
}

/*******************************************************************\

Function: xmlt::do_indent

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::do_indent(std::ostream &out, unsigned indent)
{
  for(unsigned i=0; i<indent; i++)
    out << ' ';
}

/*******************************************************************\

Function: xmlt::find

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

xmlt::elementst::const_iterator xmlt::find(const std::string &name) const
{
  for(elementst::const_iterator it=elements.begin();
      it!=elements.end();
      it++)
    if(it->name==name)
      return it;

  return elements.end();
}

/*******************************************************************\

Function: xmlt::find

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

xmlt::elementst::iterator xmlt::find(const std::string &name)
{
  for(elementst::iterator it=elements.begin();
      it!=elements.end();
      it++)
    if(it->name==name)
      return it;

  return elements.end();
}

/*******************************************************************\

Function: xmlt::set_attribute

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::set_attribute(
  const std::string &attribute,
  unsigned value)
{
  set_attribute(attribute, i2string(value));
}

/*******************************************************************\

Function: xmlt::set_attribute

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::set_attribute(
  const std::string &attribute,
  unsigned long value)
{
  set_attribute(attribute, i2string(value));
}

/*******************************************************************\

Function: xmlt::set_attribute

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void xmlt::set_attribute(
  const std::string &attribute,
  const std::string &value)
{
  if ((value[0]=='\"' && value[value.size()-1]=='\"') ||
      (value[0]=='\'' && value[value.size()-1]=='\''))
  {
    attributes[attribute]=value.substr(1,value.size()-2);
  }
  else
  {
    attributes[attribute]=value;
  }
}

/*******************************************************************\

Function: xmlt::unescape

  Inputs: a string

 Outputs: the unescaped string

 Purpose: takes a string and unescapes any xml style escaped symbols

\*******************************************************************/

std::string xmlt::unescape(const std::string &str)
{
  std::string result("");

  result.reserve(str.size());

  for(std::string::const_iterator it=str.begin();
      it!=str.end();
      it++)
  {
    if(*it=='&')
    {
      std::string tmp;
      it++;

      while(it!=str.end() && *it!=';')
        tmp+=*it++;

      if(tmp=="gt") result+='>';
      else if(tmp=="lt") result+='<';
      else if(tmp=="amp") result+='&';
      else if(tmp[0]=='#' && tmp[1]!='x')
      {
        char c=unsafe_string2int(tmp.substr(1, tmp.size()-1));
        result+=c;
      }
      else
        throw "XML escape code not implemented";
    }
    else
      result+=*it;
  }

  return result;
}
