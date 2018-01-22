#ifndef LIBPAS_DSTRUCTURE_AUTO_H
#define LIBPAS_DSTRUCTURE_AUTO_H

#define DEC_LIST(T) typedef struct {T *item; size_t length;size_t max_length;} T##List;
#define DEC_LLIST(T) typedef struct {T *top; T *last; size_t length;} T##List;

#define RESET_LIST(list) (list)->item=NULL; (list)->length=0; (list)->max_length=0;
#define FREE_LIST(list) free((list)->item); RESET_LIST(list)

#define FUN_LIST_INIT(T) int init ## T ## List(T ## List *list, unsigned int n){list->max_length=list->length=0;list->item=NULL;if(n<=0){list->max_length=0;return 1;}list->item = calloc(n, sizeof *(list->item));if (list->item == NULL) {return 0;}(list)->max_length=n;return 1;}
#define FUN_LIST_GET_BY(V,T) T *get##T##By_##V (int id, const T##List *list) {  LIST_GET_BY(V) }
#define FUN_LIST_GET_BY_ID(T) T *get ## T ## ById(int id, const T ## List *list) {  LIST_GET_BY_ID }
#define FUN_LIST_GET_BY_IDSTR(T) T *get ## T ## ById(char *id, const T ## List *list) {  LIST_GET_BY_IDSTR }
#define FUN_LLIST_GET_BY_ID(T) T *get ## T ## ById(int id, const T ## List *list) {  LLIST_GET_BY_ID(T) }

#define DEC_FUN_LIST_INIT(T)  extern int init ## T ## List(T ## List *list, unsigned int n);
#define DEC_FUN_LIST_GET_BY_ID(T) extern T *get ## T ## ById(int id, const T ## List *list);
#define DEC_FUN_LIST_GET_BY_IDSTR(T) extern T *get ## T ## ById(char *id, const T ## List *list);
#define DEC_FUN_LLIST_GET_BY_ID(T) extern T *get ## T ## ById(int id, const T ## List *list);




#define DEC_FIFO_LIST(T) struct fifo_item_ ## T {T data;int free;struct fifo_item_ ## T *prev;struct fifo_item_ ## T *next;};typedef struct fifo_item_ ## T FIFOItem_ ## T;typedef struct {FIFOItem_ ## T *item;size_t length;FIFOItem_ ## T *push_item;FIFOItem_ ## T *pop_item;Mutex mutex;} FIFOItemList_ ## T;
#define FUN_FIFO_PUSH(T) int T ## _fifo_push(T item, FIFOItemList_ ## T *list) {if (!lockMutex(&list->mutex)) {return 0;}if (list->push_item == NULL) {unlockMutex(&list->mutex);return 0;}list->push_item->data = item;list->push_item->free = 0;if(list->pop_item==NULL){list->pop_item=list->push_item;}if (list->push_item->next->free) {list->push_item = list->push_item->next;} else {list->push_item = NULL;}unlockMutex(&list->mutex);return 1;}
#define FUN_FIFO_POP(T) int T ## _fifo_pop(T * item, FIFOItemList_ ## T *list) {if (!lockMutex(&list->mutex)) {return 0;}if (list->pop_item == NULL) {unlockMutex(&list->mutex);return 0;}*item = list->pop_item->data;list->pop_item->free = 1;if (list->push_item == NULL) {list->push_item = list->pop_item;}if (!list->pop_item->next->free) {list->pop_item = list->pop_item->next;} else {list->pop_item = NULL;}unlockMutex(&list->mutex);return 1;}
#define FREE_FIFO(fifo) free((fifo)->item); (fifo)->item=NULL; (fifo)->length=0; (fifo)->pop_item = NULL;(fifo)->push_item = NULL;
#define DEC_FUN_FIFO_PUSH(T) extern int T ## _fifo_push(T item, FIFOItemList_ ## T *list);
#define DEC_FUN_FIFO_POP(T) extern int T ## _fifo_pop(T * item, FIFOItemList_ ## T *list);


#define FUN_PIPE_POP(T) T pipe_pop(T ## List *list) {return list->item[list->length-1];}
#define FUN_PIPE_PUSH(T) void pipe_push(T ## List *list, T value) {for (int i = list->length - 1; i > 0; i--) {list->item[i] = list->item[i - 1];}list->item[0] = value;}

//round list. we will push first to free place, if no free place, we will update oldest items
#define DEC_RLIST(T) typedef struct {T *item;size_t next_ind;size_t length;} T ## RList;
#define FUN_RLIST_PUSH(T) void push ## T ## RList(T ## RList *list, T value) {if (list->length<=0){return;}list->item[list->next_ind] = value;if (list->next_ind < list->length - 1) {list->next_ind++;} else {list->next_ind = 0;}}
#define FUN_RLIST_INIT(T) int init ## T ## RList(T ## RList *list, size_t n) {list->length=0;list->item=NULL;if(n<=0){return 1;}list->item = calloc(n, sizeof *(list->item));if (list->item == NULL) {return 0;}list->length=n;return 1;}
#define RESET_RLIST(list) (list)->item=NULL; (list)->length=0;
#define FREE_RLIST(list) free((list)->item); RESET_RLIST(list)

#endif 

