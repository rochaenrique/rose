////////////////////////////////////////////////////
// NOTE(erb): events

Event *
insert_event(App_State *app_state, Event_ID id, String8 title, Date_Time start, Date_Time end)
{
  Assert(app_state);
  Event *new_event = push_event(&app_state->event_arena, &app_state->event_list, 
                                &app_state->name_chunk_arena,
                                &app_state->first_free_name_chunk, 
                                title, 
                                id,
                                start, end);
  return(new_event);
}

// NOTE(erb): tags

Tag_ID
id_from_tag(App_State *app_state, Event_Tag *tag)
{
  Assert(tag - app_state->tags >= 0 && tag <= app_state->tags + ArraySize(app_state->tags)-1);
  Tag_ID id = tag_id_make((U32)(tag - app_state->tags)+1);
  return(id);
}

Event_Tag *
event_tag_from_id(App_State *app_state, Tag_ID id)
{
  Assert(id.u32 >= 1 && id.u32 <= ArraySize(app_state->tags));
  Event_Tag *result = (app_state->tags + id.u32-1);
  Assert(result != 0);
  Assert(result->used);
  return(result);
}

B32
event_has_tag(Event *event, Tag_ID tag)
{
  B32 exists = false;
  for(EachU32(idx, event->tag_count))
  {
    if(tag_id_equals(event->tags[idx], tag))
    {
      exists = true;
      break;
    }
  }
  return(exists);
}

void
event_push_tag(Event *event, Tag_ID tag)
{
  if(AssertOrBool(!event_has_tag(event, tag)))
  {
    if(AssertOrBool(event->tag_count + 1 < ArraySize(event->tags)))
    {
      event->tags[event->tag_count++] = tag;
    }
  }
}

Tag_ID 
push_event_tag(App_State *app_state, String8 name, V4f color)
{
  Tag_ID result = tag_id_null();
  Event_Tag *tag = app_state->first_free_tag;
  if(AssertOrBool(tag != 0))
  {
    SLLStackPop(app_state->first_free_tag);
    Assert(name.size <= ArraySize(tag->name));
    
    tag->name_size = (U32)CapTop(name.size, ArraySize(tag->name));
    ArrayCopy(name.value, tag->name, tag->name_size);
    
    tag->color = color;
    tag->used = true;
    result = id_from_tag(app_state, tag);
  }
  return(result);
}

////////////////////////////////////////////////////
// NOTE(erb): view

View_ID
view_id_null()
{
  View_ID result = {0};
  return(result);
}

B32
view_id_is_valid(View_ID id)
{
  B32 result = (id.u32 != 0);
  return(result);
}

B32
view_id_equals(View_ID a, View_ID b)
{
  B32 result = (a.u32 == b.u32);
  return(result);
}

U32
hash_from_view_id(View_ID id)
{
  U32 result = id.u32;
  return(result);
}

View *
view_from_id(App_State *app_state, View_ID id)
{
  Assert(id.u32 >= 1 && id.u32 <= ArraySize(app_state->views));
  View *view = (app_state->views + id.u32-1);
  return(view);
}

View_ID 
id_from_view(App_State *app_state, View *view)
{
  Assert(view - app_state->views >= 0 && view <= app_state->views + ArraySize(app_state->views)-1);
  View_ID id = {0};
  id.u32 = (U32)(view - app_state->views)+1;
  return(id);
}

View *
push_view(App_State *app_state)
{
  View *view = app_state->first_free_view;
  Assert(view);
  SLLStackPop(app_state->first_free_view);
  ZeroStruct(view);
  return(view);
}

void
view_release(App_State *app_state, View_ID id)
{
  if(view_id_is_valid(id))
  {
    View *view = view_from_id(app_state, id);
    SLLStackPush(app_state->first_free_view, view);
  }
}

String8
label_from_view_kind(View_Kind kind) 
{
  Assert(kind >= 0 && kind < ViewKind_COUNT);
  String8 view_kind_labels[] = 
  {
    [ViewKind_Calendar   ] = S8Lit("Calendar"),
    [ViewKind_Lister     ] = S8Lit("Lister"),
    [ViewKind_Table      ] = S8Lit("Table"),
    [ViewKind_Watch      ] = S8Lit("Watch"),
    [ViewKind_EventFocus ] = S8Lit("New Event"),
    [ViewKind_Tags       ] = S8Lit("Tags"),
  };
  Assert(ArraySize(view_kind_labels) == ViewKind_COUNT);
  String8 result = view_kind_labels[kind];
  return(result);
}


#if 0
// NOTE(erb): view columns
void
columns_equal_size(View *view)
{
  Assert(view);
  if(view->column_count > 0)
  {
    F32 pct = 1.0f/view->column_count;
    for(EachU32(idx, view->column_count))
    {
      view->columns[idx].pct_of_row = pct;
    }
  }
}

void
columns_remove_and_resize(View *view, U32 column_idx)
{
  Assert(view);
  Assert(column_idx >= 0 && column_idx < view->column_count);
  
  F32 removed_pct_of_row = view->columns[column_idx].pct_of_row;
  for(U32 idx = column_idx; idx < view->column_count; ++idx)
  {
    view->columns[idx] = view->columns[idx+1];
  }
  view->column_count--;
  for(EachU32(idx, view->column_count))
  {
    view->columns[idx].pct_of_row /= (1.0f - removed_pct_of_row);
  }
}

Column *
columns_append(View *view)
{
  Assert(view->column_count+1 <= ArraySize(view->columns));
  Column *new_col = (view->columns + view->column_count++);
  ZeroStruct(new_col);
  return(new_col);
}

Column *
columns_append_and_resize(View *view, F32 pct_of_row)
{
  Assert(view);
  Assert(pct_of_row > 0 && pct_of_row < 1);
  for(EachU32(idx, view->column_count))
  {
    view->columns[idx].pct_of_row *= (1.0f-pct_of_row);
  }
  Column *new_col = columns_append(view);
  new_col->pct_of_row = pct_of_row;
  return(new_col);
}
#endif

////////////////////////////////////////////////////
// NOTE(erb): panels & tabs

Tab *
panel_tab_from_view(Panel *panel, View_ID view)
{
  Tab *result = 0;
  for(EachNode(Tab, test, panel->first_tab))
  {
    if(view_id_equals(test->view, view))
    {
      result = test;
      break;
    }
  }
  return(result);
}

Tab *
panel_push_tab(Panel *panel, View_ID view)
{
  Tab *tab = panel->first_free_tab;
  if(AssertOrBool(tab != 0))
  {
    SLLStackPop(panel->first_free_tab);
  }
  ZeroStruct(tab);
  tab->view = view;
  
  return(tab);
}

Tab *
panel_push_unique_tab(Panel *panel, View_ID view)
{
  Tab *tab = 0;
  if(panel_tab_from_view(panel, view) == 0)
  {
    tab = panel_push_tab(panel, view);
    DLLQueueAppend(panel->first_tab, panel->last_tab, tab);
  }
  return(tab);
}

B32
panel_move_or_insert_view_set_current(Panel *panel, View_ID view, View_ID ref_view, B32 before_ref)
{
  Assert(panel);
  Assert(view_id_is_valid(view));
  
  B32 moved = false;
  
  panel->current_view = view;
  Tab *ref_tab = panel_tab_from_view(panel, ref_view);
  Tab *tab = panel_tab_from_view(panel, view);
  if(tab == 0)
  {
    tab = panel_push_tab(panel, view);
  }
  else if(ref_tab != 0)
  {
    moved = true;
    DLLRemove(panel->first_tab, panel->last_tab, tab);
  }
  Assert(tab);
  
  if(ref_tab == 0)
  {
    DLLQueueAppend(panel->first_tab, panel->last_tab, tab);
  }
  else
  {
    panel->current_view = view;
    if(before_ref)
    {
      Tab *ref_prev = ref_tab->prev;
      if(ref_prev)
      {
        ref_prev->next = tab;
      }
      else
      {
        panel->first_tab = tab;
      }
      tab->prev = ref_prev;
      tab->next = ref_tab;
      ref_tab->prev = tab;
    }
    else
    {
      Tab *ref_next = ref_tab->next;
      if(ref_next)
      {
        ref_next->prev = tab;
      }
      else
      {
        panel->last_tab = tab;
      }
      tab->prev = ref_tab;
      tab->next = ref_next;
      ref_tab->next = tab;
    }
  }
  return(moved);
}

void
panel_append_view_set_current(Panel *panel, View_ID view)
{
  Assert(panel);
  Assert(view_id_is_valid(view));
  Tab *tab = panel_push_tab(panel, view);
  Assert(tab);
  DLLQueueAppend(panel->first_tab, panel->last_tab, tab);
  panel->current_view = view;
}

void
panel_remove_tab(Panel *panel, Tab *tab)
{
  Assert(tab >= panel->tab_storage
         && tab <= (panel->tab_storage+ArraySize(panel->tab_storage)-1));
  Assert(tab);
  DLLRemove(panel->first_tab, panel->last_tab, tab);
  SLLStackPush(panel->first_free_tab, tab);
}

void
panel_remove_view_reset_current(Panel *panel, View_ID view)
{
  Tab *tab = panel_tab_from_view(panel, view);
  if(AssertOrBool(tab != 0))
  {
    if(tab->prev != 0)
    {
      panel->current_view = tab->prev->view;
    }
    else if(tab->next != 0)
    {
      panel->current_view = tab->next->view;
    }
    else
    {
      panel->current_view = view_id_null();
    }
    panel_remove_tab(panel, tab);
  }
}


typedef struct Panel_Rec Panel_Rec;
struct Panel_Rec
{
  Panel *next;
  U32 push_count;
  U32 pop_count;
};

Panel_Rec
panel_rec_depth_first_pre_order(Panel *panel)
{
  Panel_Rec rec = {0};
  if(panel->first != 0)
  {
    rec.next = panel->first;
    rec.push_count += 1;
  }
  else for(Panel *p = panel; p != 0; p = p->parent)
  {
    if(p->next != 0)
    {
      rec.next = p->next;
      break;
    }
    rec.pop_count += 1;
  }
  return(rec);
}

Range_V2f
rect_from_panel_child_rect(Panel *child, Range_V2f parent_rect)
{
  Assert(child);
  Range_V2f result = parent_rect;
  Panel *parent = child->parent;
  if(parent != 0)
  {
    V2f parent_rect_size = rng_v2f_size(parent_rect);
    F32 axis_size = parent_rect_size.c[parent->split_axis];
    result.max.c[parent->split_axis] = result.min.c[parent->split_axis];
    for(EachNode(Panel, p, parent->first))
    {
      if(p == child)
      {
        break;
      }
      result.min.c[parent->split_axis] += p->pct_of_parent*axis_size;
      result.max.c[parent->split_axis] = result.min.c[parent->split_axis];
    }
    result.max.c[parent->split_axis] += child->pct_of_parent*axis_size;
  }
  return(result);
}

Range_V2f
rect_from_panel(Panel *panel, Range_V2f root)
{
  Scratch scratch = scratch_begin(0, 0);
  typedef struct Walk_Node Walk_Node;
  struct Walk_Node
  {
    Panel *panel;
    Walk_Node *next;
  };
  Walk_Node *top_parent_walk_node = 0;
  // NOTE(erb): walk up the parent tree
  for(Panel *p = panel; p != 0 && p->parent != 0; p = p->parent)
  {
    Walk_Node *node = PushStruct(scratch.arena, Walk_Node);
    node->panel = p;
    SLLStackPush(top_parent_walk_node, node);
  }
  // NOTE(erb): walk down the parent tree and apply ranges
  Range_V2f result = root;
  for(EachNode(Walk_Node, node, top_parent_walk_node))
  {
    result = rect_from_panel_child_rect(node->panel, result);
  }
  scratch_end(&scratch);
  return(result);
}

Panel *
push_panel(Arena *arena, Panel **first_free, Panel *parent)
{
  Panel *child = *first_free;
  if(child != 0)
  {
    SLLStackPop(*first_free);
  }
  else
  {
    child = PushStruct(arena, Panel);
  }
  ZeroStruct(child);
  if(parent != 0)
  {
    DLLQueueAppend(parent->first, parent->last, child);
    parent->child_count += 1;
  }
  child->parent = parent;
  {
    // NOTE(erb): init tab storage
    for(EachElement(Tab, tab, child->tab_storage))
    {
      tab->next = tab+1;
    }
    child->first_free_tab = child->tab_storage;
  }
  return(child);
}

void 
release_panel(Panel *panel, Panel **first_free)
{
  // TODO(erb): actuall tree removal
  Panel *parent = panel->parent;
  if(parent != 0)
  {
    DLLRemove(parent->first, parent->last, panel);
    parent->child_count--;
    
    // NOTE(erb): event out parent children
    if(parent->child_count >= 1)
    {
      Assert(panel->first != 0);
      F32 pct = 1.0f/parent->child_count;
      for(EachNode(Panel, p, parent->first))
      {
        p->pct_of_parent = pct;
      }
    }
  }
  ZeroStruct(panel);
  SLLStackPush(*first_free, panel);
}