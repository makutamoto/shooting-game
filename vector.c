#include<windows.h>

#include "./include/vector.h"

Vector initVector() {
	Vector vector = {
		.length = 0,
		.firstItem = NULL,
		.currentItem = NULL,
		.lastItem = NULL,
	};
	return vector;
}

void resetIteration(Vector *vector) {
	vector->currentItem = NULL;
}

void* nextData(Vector *vector) {
	if(vector->currentItem == NULL) {
		if(vector->firstItem == NULL) {
			vector->currentItem = NULL;
			return NULL;
		}
		vector->currentItem = vector->firstItem;
	} else {
		if(vector->currentItem->nextItem == NULL) {
			vector->currentItem = NULL;
			return NULL;
		}
		vector->currentItem = vector->currentItem->nextItem;
	}
	return vector->currentItem->data;
}

void* previousData(Vector *vector) {
	if(vector->currentItem == NULL) {
		if(vector->lastItem == NULL) {
			vector->currentItem = NULL;
			return NULL;
		}
		vector->currentItem = vector->lastItem;
	} else {
		if(vector->currentItem->previousItem == NULL) {
			vector->currentItem = NULL;
			return NULL;
		}
		vector->currentItem = vector->currentItem->previousItem;
	}
	return vector->currentItem->data;
}

BOOL push(Vector *vector, void *data) {
	VectorItem *newItem = (VectorItem*)malloc(sizeof(VectorItem));
	if(newItem == NULL) return FALSE;
	newItem->previousItem = vector->lastItem;
	newItem->nextItem = NULL;
	newItem->data = data;
	if(vector->firstItem == NULL) vector->firstItem = newItem;
	if(vector->lastItem != NULL) vector->lastItem->nextItem = newItem;
	vector->lastItem = newItem;
	vector->length += 1;
	return TRUE;
}

void pop(Vector *vector) {
	VectorItem *previousItem = vector->lastItem->previousItem;
	free(vector->lastItem->data);
	free(vector->lastItem);
	if(previousItem == NULL) {
		vector->firstItem = NULL;
		vector->currentItem = NULL;
		vector->lastItem = NULL;
	} else {
		if(vector->currentItem == vector->lastItem) vector->currentItem = NULL;
		previousItem->nextItem = NULL;
		vector->lastItem = previousItem;
	}
	vector->length -= 1;
}

VectorItem* ItemAt(Vector vector, size_t index) {
	if(vector.firstItem == NULL) return NULL;
	VectorItem *currentItem = vector.firstItem;
	size_t i;
	for(i = 0;i < index;i++) {
		if(currentItem->nextItem == NULL) return NULL;
		currentItem = currentItem->nextItem;
	}
	return currentItem;
}

void* dataAt(Vector vector, size_t index) {
	VectorItem *item = ItemAt(vector, index);
	if(item == NULL) return NULL;
	return item->data;
}

BOOL insertAt(Vector *vector, size_t index, void *data) {
	if(vector->firstItem  == NULL) {
		if(index != 0) return FALSE;
		VectorItem *newItem = (VectorItem*)malloc(sizeof(VectorItem));
		newItem->previousItem = NULL;
		newItem->nextItem = NULL;
		newItem->data = data;
		vector->firstItem = newItem;
		vector->lastItem = newItem;
	} else {
		if(index == vector->length) {
			push(vector, data);
		} else {
			VectorItem *item = ItemAt(*vector, index);
			VectorItem *newItem;
			if(item == NULL) return FALSE;
			newItem = (VectorItem*)malloc(sizeof(VectorItem));
			newItem->previousItem = item->previousItem;
			newItem->nextItem = item;
			newItem->data = data;
			item->previousItem->nextItem = newItem;
			item->previousItem = newItem;
		}
	}
	vector->length += 1;
	return TRUE;
}

void drainAt(Vector *vector, size_t index, void **data) {
	VectorItem *item = ItemAt(*vector, index);
	if(vector->currentItem == item) vector->currentItem = item->nextItem;
	item->previousItem->nextItem = item->nextItem;
	item->nextItem->previousItem = item->previousItem;
	*data = item->data;
	free(item);
	vector->length -= 1;
}

void removeAt(Vector *vector, size_t index) {
	void *data;
	drainAt(vector, index, &data);
	free(data);
}

void clearVector(Vector *vector) {
	while(vector->length > 0) pop(vector);
}