#define TID(v) { .u32 = v, }
IR_Type_ID ir_builtin_type_id_u32  = TID(1);
IR_Type_ID ir_builtin_type_id_b32  = TID(2);
IR_Type_ID ir_builtin_type_id_u8   = TID(3);
IR_Type_ID ir_builtin_type_id_str8 = TID(4);
#undef TID

U32 ir_array_data_field_idx = 0;
U32 ir_array_count_field_idx = 1;


IR_Type_ID
ir_type_id_null()
{
  IR_Type_ID result = {0};
  return(result);
}

B32
ir_type_id_equals(IR_Type_ID a, IR_Type_ID b)
{
  B32 result = (a.u32 == b.u32);
  return(result);
}

B32
ir_type_id_is_null(IR_Type_ID id)
{
  B32 result = ir_type_id_equals(id, ir_type_id_null());
  return(result);
}

IR_Type_ID
ir_type_id_from_iden(IR_Symbol_Type *first_symbol_type, String8 iden)
{
  IR_Type_ID result = {0};
  for(EachNode(IR_Symbol_Type, test, first_symbol_type))
  {
    if(str8_equals(iden, test->name))
    {
      result = test->type;
      break;
    }
  }
  return(result);
}

IR_Type_ID
ir_type_id_from_ast_primary(IR_Symbol_Type *first_symbol_type, AST_Node *prim)
{
  Assert(prim->kind == ASTKind_Primary);
  
  IR_Type_ID result = {0};
  switch(prim->prim.kind)
  {
    case ASTPrimaryKind_Identifier:
    {
      result = ir_type_id_from_iden(first_symbol_type, prim->prim.str8);
    } break;
    
    case ASTPrimaryKind_NumLiteral:
    {
      result = ir_builtin_type_id_u32;
    } break;
    
    case ASTPrimaryKind_StrLiteral:
    {
      result = ir_builtin_type_id_str8;
    } break;
  }
  return(result);
}

IR_Type *
ir_type_table_next(IR_Type_Table *table, IR_Type_Kind kind)
{
  IR_Type *result = 0;
  if(AssertOrBool(table->count+1 <= ArraySize(table->types)))
  {
    result = (table->types + table->count++);
    result->id.u32 = table->count;
    result->kind = kind;
  }
  return(result);
}

IR_Field_Type *
ir_type_next_field(IR_Type *type, String8 name, IR_Type_ID field_type_id)
{
  IR_Field_Type *field = (type->fields + type->field_count++);
  field->name = name;
  field->type = field_type_id;
  return(field);
}

IR_Field_Type *
ir_type_append_field_packed(IR_Type *type, String8 name, IR_Type *field_type)
{
  IR_Field_Type *field = ir_type_next_field(type, name, field_type->id);
  field->byte_offset = type->byte_count;
  type->byte_count += field_type->byte_count;
  return(field);
}

IR_Type *
ir_type_from_id(IR_Type_Table *table, IR_Type_ID id)
{
  IR_Type *result = 0;
  if(AssertOrBool(id.u32 >= 1 && id.u32-1 < table->count))
  {
    result = (table->types + (id.u32-1));
    Assert(result->kind != IRTypeKind_None);
  }
  return(result);
}

IR_Type *
ir_type_make_address_of(IR_Type_Table *table, IR_Type_ID type)
{
  IR_Type *result = ir_type_table_next(table, IRTypeKind_AddressOf);
  result->sub_type = type;
  result->byte_count = sizeof(void *);
  return(result);
}

IR_Type *
ir_type_make_array(IR_Type_Table *table, IR_Type_ID data_type)
{
  IR_Type *array = ir_type_table_next(table, IRTypeKind_Record);
  array->is_array_record = true;
  array->byte_count = sizeof(IR_Array);
  
  IR_Type *data = ir_type_make_address_of(table, data_type);
  
  Assert(ir_array_data_field_idx == 0);
  IR_Field_Type *data_field = ir_type_next_field(array, S8("data"), data->id);
  data_field->byte_offset = offsetof(IR_Array, data);
  
  Assert(ir_array_count_field_idx == 1);
  IR_Field_Type *count_field = ir_type_next_field(array, S8("count"), ir_builtin_type_id_u32);
  count_field->byte_offset = offsetof(IR_Array, count);
  return(array);
}

void
ir_type_table_init(IR_Type_Table *table)
{
  Assert(!table->initialized);
  table->initialized = true;
  
  {
    IR_Type *u32 = ir_type_table_next(table, IRTypeKind_U32);
    u32->byte_count = sizeof(U32);
    Assert(ir_type_id_equals(ir_builtin_type_id_u32, u32->id));
  }
  
  {
    IR_Type *b32 = ir_type_table_next(table, IRTypeKind_B32);
    b32->byte_count = sizeof(B32);
    Assert(ir_type_id_equals(ir_builtin_type_id_b32, b32->id));
  }
  
  {
    IR_Type *u8 = ir_type_table_next(table, IRTypeKind_U8);
    u8->byte_count = sizeof(U8);
    Assert(ir_type_id_equals(ir_builtin_type_id_u8, u8->id));
  }
  
  {
    IR_Type *str8 = ir_type_make_array(table, ir_builtin_type_id_u8);
    Assert(ir_type_id_equals(ir_builtin_type_id_str8, str8->id));
    str8->record_name = S8("Str8");
  }
}

IR_Op *
ir_push_op(Arena *arena, IR_Op_List *list, IR_Op_Kind kind, IR_Type_ID type)
{
  IR_Op *op = PushStruct(arena, IR_Op);
  op->kind = kind;
  op->type = type;
  SLLQueueAppend(list->first, list->last, op);
  list->count++;
  return(op);
}

E_Value
e_value_slice_as(E_Value *root, IR_Type *slice_type, U32 byte_offset)
{
  Assert(byte_offset + slice_type->byte_count <= root->memory_size);
  E_Value result = {0};
  result.type = slice_type->id;
  result.memory_start = (root->memory_start + byte_offset);
  result.memory_size = slice_type->byte_count;
  return(result);
}

IR_Field_Type *
ir_type_field_from_name(IR_Type *type, String8 name)
{
  IR_Field_Type *field = 0;
  for(EachPtr(IR_Field_Type, test, type->fields, type->field_count))
  {
    if(str8_equals(test->name, name))
    {
      field = test;
      break;
    }
  }
  return(field);
}

E_Value
e_value_from_field_name(E_Value *root, String8 name, IR_Type_Table *type_table)
{
  IR_Type *type = ir_type_from_id(type_table, root->type); // TODO(erb): maybe this shouldnt be here?
  Assert(type != 0);
  
  Assert(root);
  IR_Field_Type *field = ir_type_field_from_name(type, name);
  Assert(field != 0); // TODO(erb): handle error here
  
  IR_Type *field_type = ir_type_from_id(type_table, field->type);
  Assert(field_type != 0);
  
  E_Value result = e_value_slice_as(root, field_type, field->byte_offset);
  return(result);
}

IR_Op *
ir_push_op_from_ast(Arena *arena, AST_Node *ast, IR_Op_List *list, IR_Type_Table *type_table,
                    IR_Symbol_Type *first_symbol_type, String8 *error)
{
  IR_Op *result = 0;
  switch(ast->kind) 
  {
    case ASTKind_Binary:
    {
      if(ast->bin.kind == ASTBinaryKind_FieldAccess)
      {
        IR_Op *record = ir_push_op_from_ast(arena, ast->bin.left, list, type_table, first_symbol_type, error);
        AssertImplies(record == 0, error->value != 0);
        
        // NOTE(erb): type check first arg to be a record
        IR_Type *record_type = 0;
        if(error->value == 0)
        {
          record_type = ir_type_from_id(type_table, record->type);
          if(record_type == 0)
          {
            *error = S8("type not found");
          }
          else if(record_type->kind != IRTypeKind_Record)
          {
            *error = S8("type is not a record");
          }
        }
        
        // NOTE(erb): type check the second arg to be a 'name for a field'
        if(error->value == 0)
        {
          if(!(ast->bin.right->kind == ASTKind_Primary && 
               ast->bin.right->prim.kind == ASTPrimaryKind_Identifier))
          {
            *error = S8("expected accessor to be the name for a field");
          }
        }
        
        // NOTE(erb): type check member in record
        IR_Field_Type *field = 0;
        if(error->value == 0)
        {
          for(EachU32(idx, record_type->field_count))
          {
            IR_Field_Type *test = (record_type->fields + idx);
            if(str8_equals(test->name, ast->bin.right->prim.str8))
            {
              field = test;
              break;
            }
          }
          if(field == 0)
          {
            *error = S8("record does not have this field");
          }
        }
        
        // NOTE(erb): field offset op
        if(error->value == 0)
        {
          // NOTE(erb): push index
          Assert(field != 0);
          IR_Op *index_op = ir_push_op(arena, list, IROpKind_LiteralU32, ir_builtin_type_id_u32);
          index_op->u32 = 1;
          
          // NOTE(erb): offset access with stride
          result = ir_push_op(arena, list, IROpKind_OffsetAccessDeref, field->type);
          result->u32 = field->byte_offset;
          result->access_kind = IRAccessKind_AddressOf;
        }
      }
      else if(ast->bin.kind == ASTBinaryKind_ArrayAccess)
      {
        IR_Op *array_ptr_op = ir_push_op_from_ast(arena, ast->bin.left, list, type_table, first_symbol_type, error);
        AssertImplies(array_ptr_op == 0, error->value != 0);
        
        // NOTE(erb): type check first arg to be a address
        IR_Type *array_ptr_type = 0;
        if(error->value == 0)
        {
          array_ptr_type = ir_type_from_id(type_table, array_ptr_op->type);
          if(array_ptr_type == 0)
          {
            *error = S8("array address type not found");
          }
          else if(array_ptr_type->kind != IRTypeKind_AddressOf)
          {
            *error = S8("type is not an address, cannot be accessed");
          }
        }
        
        // NOTE(erb): sub_type
        IR_Type *sub_type = 0;
        if(error->value == 0)
        {
          sub_type = ir_type_from_id(type_table, array_ptr_type->sub_type);
          if(sub_type == 0)
          {
            *error = S8("array subtype not found");
          }
        }
        
        // NOTE(erb): parse index op
        IR_Op *index_op = 0;
        if(error->value == 0)
        {
          index_op = ir_push_op_from_ast(arena, ast->bin.right, list, type_table, first_symbol_type, error);
          AssertImplies(index_op == 0, error->value != 0);
        }
        
        // NOTE(erb): type check the second arg to be a 'int'
        if(error->value == 0)
        {
          Assert(index_op != 0);
          if(!ir_type_id_equals(ir_builtin_type_id_u32, index_op->type))
          {
            *error = S8("expected index type to be an int");
          }
        }
        
        // NOTE(erb): produce op
        if(error->value == 0)
        {
          // NOTE(erb): index is allready in stack
          // NOTE(erb): push offset with stride
          result = ir_push_op(arena, list, IROpKind_OffsetAccessDeref, sub_type->id);
          result->u32 = sub_type->byte_count;
          result->access_kind = IRAccessKind_Direct;
        }
      }
      else
      {
        IR_Type_ID target_type_id = ((ast->bin.kind == ASTBinaryKind_LogicAnd || 
                                      ast->bin.kind == ASTBinaryKind_LogicOr)
                                     ? ir_builtin_type_id_b32 : ir_builtin_type_id_u32);
        
        IR_Op *left_op = ir_push_op_from_ast(arena, ast->bin.left, list, type_table, first_symbol_type, error);
        AssertImplies(left_op == 0, error->value != 0);
        if(error->value == 0 && !ir_type_id_equals(left_op->type, target_type_id))
        {
          *error = S8("left argument type not suited for operation");
        }
        
        IR_Op *right_op = 0;
        if(error->value == 0)
        {
          right_op = ir_push_op_from_ast(arena, ast->bin.right, list, type_table, first_symbol_type, error);
          AssertImplies(right_op == 0, error->value != 0);
        }
        if(error->value == 0 && !ir_type_id_equals(right_op->type, target_type_id))
        {
          *error = S8("right argument type not suited for operation");
        }
        
        IR_Op_Kind op_kind = IROpKind_None;
        if(error->value == 0)
        {
          switch(ast->bin.kind)
          {
            case ASTBinaryKind_Sum: op_kind = IROpKind_Sum; break;
            case ASTBinaryKind_Mul: op_kind = IROpKind_Mul; break;
            case ASTBinaryKind_Sub: op_kind = IROpKind_Sub; break;
            case ASTBinaryKind_Div: op_kind = IROpKind_Div; break;
            case ASTBinaryKind_Mod: op_kind = IROpKind_Mod; break;
            case ASTBinaryKind_Eq : op_kind = IROpKind_Eq ; break;
            case ASTBinaryKind_Gt : op_kind = IROpKind_Gt ; break;
            case ASTBinaryKind_Ge : op_kind = IROpKind_Ge ; break;
            case ASTBinaryKind_Lt : op_kind = IROpKind_Lt ; break;
            case ASTBinaryKind_Le : op_kind = IROpKind_Le ; break;
            case ASTBinaryKind_BitAnd: op_kind = IROpKind_BitAnd; break;
            case ASTBinaryKind_BitOr: op_kind = IROpKind_BitOr; break;
            case ASTBinaryKind_LogicAnd: op_kind = IROpKind_LogicAnd; break;
            case ASTBinaryKind_LogicOr: op_kind = IROpKind_LogicOr; break;
            InvalidDefaultCase;
          }
          
          result = ir_push_op(arena, list, op_kind, target_type_id);
        }
      }
      
    } break;
    
    case ASTKind_Primary:
    {
      switch(ast->prim.kind)
      {
        case ASTPrimaryKind_Identifier:
        {
          IR_Type_ID type = ir_type_id_from_iden(first_symbol_type, ast->prim.str8);
          if(ir_type_id_is_null(type))
          {
            *error = S8("primary type not found");
          }
          else
          {
            result = ir_push_op(arena, list, IROpKind_Identifier, type);
            result->str8 = ast->prim.str8;
          }
        } break;
        
        case ASTPrimaryKind_NumLiteral:
        {
          result = ir_push_op(arena, list, IROpKind_LiteralU32, ir_builtin_type_id_u32);
          result->u32 = ast->prim.u32;
        } break;
        
        case ASTPrimaryKind_StrLiteral:
        {
          result = ir_push_op(arena, list, IROpKind_LiteralStr8, ir_builtin_type_id_str8);
          result->str8 = ast->prim.str8;
        } break;
      }
    } break;
    
    case ASTKind_Function:
    {
      // NOTE(erb): validate function type
      IR_Type_ID func_type_id = ir_type_id_from_iden(first_symbol_type, ast->func.name);
      if(ir_type_id_is_null(func_type_id))
      {
        *error = S8("function type not found");
      }
      
      IR_Type *type = 0;
      if(error->value == 0)
      {
        type = ir_type_from_id(type_table, func_type_id);
        if(type == 0)
        {
          *error = S8("type not found");
        }
        else if(type->kind != IRTypeKind_Function)
        {
          *error = S8("type is not a function");
        }
      }
      
      // NOTE(erb): check arg count
      if(error->value == 0 && ast->func.args.count != type->field_count)
      {
        *error = S8("incorrect number of args");
      }
      
      // NOTE(erb): type check arg types
      if(error->value == 0)
      {
        U32 field_idx = 0;
        for(EachNode(AST_Node, arg, ast->func.args.first))
        {
          IR_Op *arg_op = ir_push_op_from_ast(arena, arg, list, type_table, first_symbol_type, error);
          
          Assert(field_idx <= type->field_count);
          IR_Field_Type *expected_arg_type = (type->fields + field_idx);
          
          if(!ir_type_id_equals(expected_arg_type->type, arg_op->type))
          {
            *error = S8("incorrect argument type");
            break;
          }
          ++field_idx;
        }
      }
      
      // NOTE(erb): push func call
      if(error->value == 0)
      {
        result = ir_push_op(arena, list, IROpKind_FunctionCall, type->sub_type);
        result->str8 = ast->func.name;
        result->func_type = type->id;
      }
    } break;
    
    InvalidDefaultCase;
  }
  return(result);
}

IR_Op_List
ir_op_list_from_ast(Arena *arena, AST_Node *ast, IR_Type_Table *type_table, 
                    IR_Symbol_Type *first_symbol_type, String8 *error)
{
  IR_Op_List list = {0};
  ir_push_op_from_ast(arena, ast, &list, type_table, first_symbol_type, error);
  return(list);
}

E_Global_Iden *
e_match_global_iden(E_Global_Iden *iden_list, String8 name, B32 is_value_not_func, IR_Type_ID type_id)
{
  E_Global_Iden *result = 0;
  for(EachNode(E_Global_Iden, iden, iden_list))
  {
    if((is_value_not_func == iden->is_value_not_func) &&
       ir_type_id_equals(iden->type, type_id) && 
       str8_equals(name, iden->name))
    {
      result = iden;
      break;
    }
  }
  return(result);
}

void *
e_value_get_bytes_from(Buffer *buff, E_Value *value)
{
  void *result = 0;
  if(AssertOrBool((value->memory_start + value->memory_size) <= buff->size))
  {
    result = ((U8 *)buff->base + value->memory_start);
  }
  return(result);
}

#define EValueGetPtr(buff, value, Type) (Assert(sizeof(Type) == (value)->memory_size), \
(Type *)e_value_get_bytes_from((buff), (value)))

IR_Array *
e_array_from_value_cast_unchecked(Buffer *buffer, E_Value *value)
{
  IR_Array *result = EValueGetPtr(buffer, value, IR_Array);
  return(result);
}

void *
e_value_set_push_memory(E_Value *value, Arena *arena, U32 byte_count)
{
  value->memory_start = safe_truncate_U64_to_U32(arena->used);
  value->memory_size = byte_count;
  void *memory = arena_push_size(arena, byte_count);
  return(memory);
}

E_Value *
e_stack_push(E_Stack *stack, IR_Type_ID id, U32 byte_count)
{
  E_Value *value = 0;
  if(AssertOrBool(stack->value_count+1 < ArraySize(stack->values) &&
                  stack->memory_arena.used + byte_count <= stack->memory_arena.buffer.size))
  {
    value = (stack->values + stack->value_count++);
    ZeroStruct(value);
    value->type = id;
    e_value_set_push_memory(value, &stack->memory_arena, byte_count);
  }
  return(value);
}

E_Value *
e_stack_push_write_type(E_Stack *stack, IR_Type *type, U8 *memory)
{
  E_Value *value = e_stack_push(stack, type->id, type->byte_count);
  Assert(value->memory_size == type->byte_count);
  U8 *dest = e_value_get_bytes_from(&stack->memory_arena.buffer, value);
  mem_copy(memory, dest, type->byte_count);
  return(value);
}

E_Stack_Snapshot
e_stack_pop(Arena *arena, E_Stack *stack, U32 pop_count)
{
  E_Stack_Snapshot result = {0};
  // TODO(erb): change this to adjust the ranges of the resulting values
  // the new values slice into another piece of memory
  
  Assert(pop_count > 0);
  if(AssertOrBool(stack->value_count >= pop_count))
  {
    // NOTE(erb): pop bytes & values
    E_Value *popped_values = stack->values + AssertOrUSub(stack->value_count, pop_count);
    E_Value first = popped_values[0];
    U64 bytes_to_pop = AssertOrUSub(stack->memory_arena.used, first.memory_start);
    Assert(first.memory_start <= stack->memory_arena.used);
    Assert((first.memory_start + first.memory_size)
           <= stack->memory_arena.used);
    
    // NOTE(erb): alloc & copy result
    result.values = PushArray(arena, pop_count, E_Value);
    result.value_count = pop_count;
    
    ArrayCopy(popped_values, result.values, pop_count);
    result.memory = push_buffer(arena, bytes_to_pop);
    U8 *src = ((U8 *)stack->memory_arena.buffer.base + first.memory_start);
    ArrayCopy(src, result.memory.base, bytes_to_pop);
    
    // NOTE(erb): adjust ranges
    for(EachPtr(E_Value, v, result.values, result.value_count))
    {
      v->memory_start -= first.memory_start;
    }
    
    // NOTE(erb): actually pop memory and values
    arena_pop_size(&stack->memory_arena, bytes_to_pop); 
    stack->value_count -= pop_count;
  }
  return(result);
}

E_Value_Interp
e_interp_from_ir_ops(Arena *arena, IR_Op_List *ir_ops, IR_Type_Table *type_table, E_Global_Iden *iden_list)
{
  Scratch scratch = scratch_begin(&arena, 1);
  
  E_Stack stack = {0};
  arena_init(&stack.memory_arena, stack.memory, ArraySize(stack.memory));
  
  for(EachNode(IR_Op, op, ir_ops->first))
  {
#if DEBUG
    if(stack.value_count > 0)
    {
      E_Value *last = (stack.values + stack.value_count-1); 
      Assert(stack.memory_arena.used == (last->memory_start + last->memory_size));
    }
#endif
    
    switch(op->kind)
    {
      case IROpKind_FunctionCall:
      {
        E_Global_Iden *func_addr = e_match_global_iden(iden_list, op->str8, false, op->func_type);
        Assert(func_addr != 0);
        
        IR_Type *func_type = ir_type_from_id(type_table, func_addr->type);
        Assert(func_type != 0);
        
        Assert(stack.value_count >= func_type->field_count && 
               stack.memory_arena.used >= func_type->byte_count);
        
        // NOTE(erb): args
        E_Value *args = stack.values + AssertOrUSub(stack.value_count, func_type->field_count);
        // NOTE(erb): call
        Scratch func_scratch = arena_get_scratch(scratch.arena);
        E_Value result = func_addr->func(func_scratch.arena, args, func_type->field_count, 
                                         &stack.memory_arena.buffer, type_table, func_addr->user_data);
        U8 *result_memory = ((U8 *)func_scratch.arena->buffer.base + result.memory_start);
        
        // NOTE(erb): pop args
        stack.value_count -= func_type->field_count;
        arena_pop_size(&stack.memory_arena, func_type->byte_count);
        
        // NOTE(erb): push result
        Assert(!ir_type_id_is_null(result.type));
        IR_Type *result_type = ir_type_from_id(type_table, result.type);
        Assert(result_type != 0 && result_type->byte_count == result.memory_size);
        e_stack_push_write_type(&stack, result_type, result_memory);
        arena_scratch_reset(&func_scratch);
      } break;
      
      case IROpKind_Identifier:
      {
        E_Global_Iden *iden = e_match_global_iden(iden_list, op->str8, true, op->type);
        Assert(iden != 0);
        IR_Type *type = ir_type_from_id(type_table, iden->type);
        Assert(type->byte_count == iden->value_bytes.size);
        e_stack_push_write_type(&stack, type, iden->value_bytes.value);
      } break;
      
      case IROpKind_OffsetAccessDeref:
      {
        Scratch op_scratch = arena_get_scratch(scratch.arena);
        U32 stride = op->u32;
        
        // NOTE(erb): pop base address and index 
        E_Stack_Snapshot pop = e_stack_pop(op_scratch.arena, &stack, 2);
        E_Value base_value = pop.values[0];
        E_Value index_value = pop.values[1];
        Assert(ir_type_id_equals(index_value.type, ir_builtin_type_id_u32));
        Assert(index_value.memory_size == sizeof(U32));
        
        // NOTE(erb): offset calculation
        U32 index = *(U32 *)e_value_get_bytes_from(&pop.memory, &index_value);
        U32 offset = (stride * index);
        
        // NOTE(erb): base address & final address
        U8 *bytes = e_value_get_bytes_from(&pop.memory, &base_value);
        U8 *base_addr = 0;
        switch(op->access_kind)
        {
          case IRAccessKind_Direct: 
          {
            base_addr = (U8 *)(*((U64 *)bytes)); // NOTE(erb): interpret the memory as an address
          } break;
          case IRAccessKind_AddressOf: 
          {
            Assert(offset < base_value.memory_size);
            base_addr = bytes;                   // NOTE(erb): slice into the memory directly
          } break; 
          InvalidDefaultCase;
        }
        U8 *memory = (base_addr + offset);
        
        // NOTE(erb): push the value
        IR_Type *result_type = ir_type_from_id(type_table, op->type); // NOTE(erb): basically a cast here
        Assert(result_type != 0);
        e_stack_push_write_type(&stack, result_type, memory);
        
        scratch_end(&op_scratch);
      } break;
      
      case IROpKind_LiteralU32:
      {
        Assert(ir_type_id_equals(op->type, ir_builtin_type_id_u32));
        IR_Type *type = ir_type_from_id(type_table, op->type);
        e_stack_push_write_type(&stack, type, (U8 *)&op->u32);
      } break;
      
      case IROpKind_LiteralStr8:
      {
        Assert(ir_type_id_equals(op->type, ir_builtin_type_id_str8));
        IR_Type *type = ir_type_from_id(type_table, op->type);
        E_Value *value = e_stack_push(&stack, op->type, type->byte_count);
        
        // NOTE(erb): get field offsets
        U32 data_field_offset = type->fields[ir_array_data_field_idx].byte_offset;
        U32 count_field_offset = type->fields[ir_array_count_field_idx].byte_offset;
        Assert(data_field_offset <= type->byte_count);
        Assert(count_field_offset <= type->byte_count);
        
        // NOTE(erb): populate fields
        U8 *base = e_value_get_bytes_from(&stack.memory_arena.buffer, value);
        U64 *data_field_memory = (U64 *)(base + data_field_offset);
        U32 *count_field_memory = (U32 *)(base + count_field_offset);
        *data_field_memory = (U64)op->str8.value;
        *count_field_memory = safe_truncate_U64_to_U32(op->str8.size);
      } break;
      
      case IROpKind_Mul:
      case IROpKind_Sub:
      case IROpKind_Div:
      case IROpKind_Mod:
      case IROpKind_Eq:
      case IROpKind_Gt:
      case IROpKind_Ge:
      case IROpKind_Lt:
      case IROpKind_Le:
      case IROpKind_BitAnd:
      case IROpKind_BitOr:
      case IROpKind_Sum:
      case IROpKind_LogicAnd:
      case IROpKind_LogicOr:
      {
        Scratch op_scratch = arena_get_scratch(scratch.arena);
        E_Stack_Snapshot pop = e_stack_pop(op_scratch.arena, &stack, 2);
        
        AssertImplies(op->kind == IROpKind_LogicAnd || op->kind == IROpKind_LogicOr, 
                      ir_type_id_equals(op->type, ir_builtin_type_id_b32));
        AssertImplies(op->kind != IROpKind_LogicAnd && op->kind != IROpKind_LogicOr, 
                      ir_type_id_equals(op->type, ir_builtin_type_id_u32));
        
        U64 a = *EValueGetPtr(&pop.memory, (pop.values+0), U32);
        U64 b = *EValueGetPtr(&pop.memory, (pop.values+1), U32);
        
        U64 result = 0;
        switch(op->kind)
        {
          case IROpKind_Sum     : result = a  + b; break;
          case IROpKind_Mul     : result = a  * b; break;
          case IROpKind_Sub     : result = a  - b; break;
          case IROpKind_Div     : result = a  / b; break;
          case IROpKind_Mod     : result = a  % b; break;
          case IROpKind_Eq      : result = a == b; break;
          case IROpKind_Gt      : result = a  < b; break;
          case IROpKind_Ge      : result = a >= b; break;
          case IROpKind_Lt      : result = a  < b; break;
          case IROpKind_Le      : result = a <= b; break;
          case IROpKind_BitAnd  : result = a  & b; break;
          case IROpKind_BitOr   : result = a  | b; break;
          case IROpKind_LogicAnd: result = a && b; break;
          case IROpKind_LogicOr : result = a || b; break;
          InvalidDefaultCase;
        }
        
        IR_Type *type = ir_type_from_id(type_table, op->type);
        e_stack_push_write_type(&stack, type, (U8 *)&result);
        
        scratch_end(&op_scratch);
      } break;
      
      InvalidDefaultCase;
    }
  }
  E_Value_Interp result = {0};
  if(AssertOrBool(stack.value_count == 1))
  {
    result.memory = push_buffer(arena, stack.memory_arena.used);
    mem_copy(stack.memory_arena.buffer.base, result.memory.base, stack.memory_arena.used);
    result.value = stack.values[0];
  }
  scratch_end(&scratch);
  return(result);
}


IR_Type_ID
ir_array_data_type(IR_Type_Table *type_table, IR_Type *array)
{
  Assert(array->kind == IRTypeKind_Record && array->is_array_record);
  IR_Field_Type *field = (array->fields + ir_array_data_field_idx);
  IR_Type *data_type = ir_type_from_id(type_table, field->type);
  IR_Type_ID result = {0};
  if(AssertOrBool(data_type != 0))
  {
    result = data_type->sub_type;
  }
  return(result);
}

E_Value
e_value_make(IR_Type_ID type, U32 memory_start, U32 memory_size)
{
  E_Value result = {0};
  result.type = type;
  result.memory_start = memory_start;
  result.memory_size = memory_size;
  return(result);
}

E_Value
e_value_from_index_offset(IR_Type *type, U32 idx)
{
  E_Value result = e_value_make(type->id, (idx * type->byte_count), type->byte_count);
  return(result);
}
