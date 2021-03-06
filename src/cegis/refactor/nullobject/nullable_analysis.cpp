#include <algorithm>
#include <functional>

#include <goto-programs/remove_returns.h>

#include <cegis/cegis-util/program_helper.h>
#include <cegis/cegis-util/string_helper.h>
#include <cegis/refactor/options/refactor_program.h>
#include <cegis/refactor/instructionset/execute_cegis_program.h>
#include <cegis/refactor/nullobject/range_collector.h>
#include <cegis/refactor/nullobject/nullable_analysis.h>

namespace
{
std::set<irep_idt> get_nullable_classes(
    const refactor_programt::sketcht::typest &types)
{
  std::set<irep_idt> result;
  for (const typet &type : types)
  {
    if (!type.get_bool(CEGIS_NULLED_TYPE_ID)) continue;
    const typet &class_type=to_pointer_type(type).subtype();
    result.insert(to_symbol_type(class_type).get_identifier());
  }
  return result;
}

bool is_member_of(const irep_idt &class_id, const irep_idt &method_id)
{
  std::string prefix(id2string(class_id));
  return starts_with(id2string(method_id), prefix+='.');
}

template<class itert>
bool is_member_of_any(itert first, const itert last, const irep_idt &meth_id)
{
  return last != std::find_if(first, last, [&meth_id](const irep_idt &class_id)
  { return is_member_of(class_id, meth_id);});
}
}

std::set<irep_idt> get_nullable_methods(const refactor_programt &prog)
{
  const symbol_tablet::symbolst &symbols=prog.st.symbols;
  std::set<irep_idt> nullable_methods;
  for (const refactor_programt::sketcht &sketch : prog.sketches)
  {
    const refactor_programt::sketcht::typest &types=sketch.types;
    const std::set<irep_idt> null_cls(get_nullable_classes(types));
    for (const typet &type : types)
      if (type.get_bool(CEGIS_NULLED_TYPE_ID))
        for (const symbol_tablet::symbolst::value_type &entry : symbols)
        {
          const symbolt &symbol=entry.second;
          if (ID_code != symbol.type.id()) continue;
          const irep_idt &name=entry.first;
          if (is_member_of_any(null_cls.begin(), null_cls.end(), name))
            nullable_methods.insert(name);
        }
  }
  return nullable_methods;
}

cegis_operand_datat slots_per_type(const symbol_tablet &st,
    const std::set<irep_idt> &state_vars)
{
  const namespacet ns(st);
  cegis_operand_datat result;
  for (const irep_idt &state_var : state_vars)
    ++result[ns.follow(st.lookup(state_var).type)];
  return result;
}

std::string get_return_value_name(const irep_idt &method)
{
  std::string ret_val_name(id2string(method));
  return ret_val_name+=RETURN_VALUE_SUFFIX;
}

cegis_operand_datat get_operand_signature(const symbol_tablet &st,
    const irep_idt &method)
{
  assert(st.has_symbol(method));
  // TODO: Add global vars
  cegis_operand_datat result;
  const code_typet &code_type=to_code_type(st.lookup(method).type);
  const typet &return_type=code_type.return_type();
  const std::string ret_val_name(get_return_value_name(method));
  if (st.has_symbol(ret_val_name)) result[st.lookup(ret_val_name).type]=1;
  for (const code_typet::parameterst::value_type &param : code_type.parameters())
    ++result[param.type()];
  return result;
}

namespace
{
bool calls_method(const goto_programt::instructiont &instr,
    const irep_idt &method)
{
  if (goto_program_instruction_typet::FUNCTION_CALL != instr.type) return false;
  const exprt &func=to_code_function_call(instr.code).function();
  if (ID_symbol != func.id()) return false;
  return method == to_symbol_expr(func).get_identifier();
}

std::vector<irep_idt> get_operands(const symbol_tablet &st,
    const irep_idt &method)
{
  const code_typet &code_type=to_code_type(st.lookup(method).type);
  const typet &return_type=code_type.return_type();
  std::vector<irep_idt> result;
  // TODO: Add globals
  const std::string ret_val_name(get_return_value_name(method));
  if (st.has_symbol(ret_val_name)) result.push_back(ret_val_name);
  for (const code_typet::parameterst::value_type &param : code_type.parameters())
    if (!param.get_this()) result.push_back(param.get_identifier());
  return result;
}
}

void replace_method_call_by_processor(const symbol_tablet &st,
    goto_programt &body, goto_programt::targett first,
    const goto_programt::targett last, const irep_idt &method,
    const std::string &processor, const std::string &prog)
{
  for (; first != last; ++first)
    if (calls_method(*first, method))
    {
      const std::vector<irep_idt> operands(get_operands(st, method));
      if (!operands.empty())
      {
        goto_programt::targett pos=insert_before_preserve_labels(body, first);
        instrument_cegis_operand(st, *pos, 0, operands.front());
        for (size_t i=1; i < operands.size(); ++i)
          pos=instrument_cegis_operand(st, body, pos, i, operands[i]);
      }
      call_processor(st, *first, processor, prog);
    }
}
