#include "markov_chain.h"
#include "string.h"
/**
 * Get random number between 0 and max_number [0, max_number).
 * @param max_number
 * @return Random numbers
 */

// Get a random number between 0 and max_number (exclusive)
int get_random_number(int max_number) {
    return rand() % max_number;
}

// Check if a word ends with a period (sentence ender)
bool is_sentence_ender(char *word) {
    if (word == NULL) return false;
    int len = strlen(word);
    // Check for period at end of word
    return (len > 0 && (word[len - 1] == '.' || 
                       word[len - 1] == '!' || 
                       word[len - 1] == '?'));
}

// Find a node in the database by its data
Node* get_node_from_database(MarkovChain *markov_chain, char *data_ptr) {
    if (markov_chain == NULL || data_ptr == NULL) {
        return NULL;
    }
    Node* currentNode = markov_chain->database->first;
    while (currentNode != NULL) {
        MarkovNode *markov_node = (MarkovNode *)currentNode->data;
        if (markov_node != NULL && strcmp(markov_node->data, data_ptr) == 0) {
            return currentNode;
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

// Add a new node to the database
Node* add_to_database(MarkovChain *markov_chain, char *data_ptr) {
    if (markov_chain == NULL || data_ptr == NULL) {
        return NULL;
    }
    Node* existing_node = get_node_from_database(markov_chain, data_ptr);
    if (existing_node != NULL) {
        return existing_node;
    }
    MarkovNode *new_node = (MarkovNode *) malloc(sizeof(MarkovNode));
    if (new_node == NULL) {
        perror("Allocation failed");
        return NULL;
    }
    new_node->data = strdup(data_ptr); // Allocate and copy the string
    if (new_node->data == NULL) {
        free(new_node);  // Free the MarkovNode if strdup fails
        perror("Allocation failed");
        return NULL;
    }
    new_node->frequency_list = NULL;
    new_node->listLen = 0;
    if (add(markov_chain->database, new_node) == 0) {
        return markov_chain->database->last;
    }
    free(new_node->data);  // Free the data if add fails
    free(new_node);        // Free the MarkovNode if add fails
    return NULL;
}

// Add a node to the frequency list of another node
int add_node_to_frequency_list(MarkovNode *first_node, MarkovNode *second_node) {
    if (first_node == NULL || second_node == NULL) {
        return 1;
    }
    if (first_node->frequency_list == NULL) {
        first_node->frequency_list = (MarkovNodeFrequency *) malloc(sizeof(MarkovNodeFrequency));
        if (first_node->frequency_list == NULL) {
            perror("Allocation failed");
            return 1;
        }
        first_node->frequency_list[0].markov_node = second_node;
        first_node->frequency_list[0].frequency = 1;
        first_node->listLen = 1;
        return 0;
    }
    for (int i = 0; i < first_node->listLen; ++i) {
        if (first_node->frequency_list[i].markov_node == second_node) {
            first_node->frequency_list[i].frequency++;
            return 0;
        }
    }
    MarkovNodeFrequency *new_list = realloc(first_node->frequency_list, sizeof(MarkovNodeFrequency) * (first_node->listLen + 1));
    if (new_list == NULL) {
        perror("Reallocation failed");
        return 1;
    }
    first_node->frequency_list = new_list;
    first_node->frequency_list[first_node->listLen].markov_node = second_node;
    first_node->frequency_list[first_node->listLen].frequency = 1;
    first_node->listLen++;
    return 0;
}

// Free the entire database
void free_database(MarkovChain **ptr_chain) {
    if (ptr_chain == NULL || *ptr_chain == NULL) {
        return;
    }
    MarkovChain *markov_chain = *ptr_chain;
    LinkedList *database = markov_chain->database;
    if (database != NULL) {
        Node *current = database->first;
        while (current != NULL) {
            Node *next = current->next;
            MarkovNode *markov_node = (MarkovNode *)current->data;
            if (markov_node != NULL) {
                free(markov_node->frequency_list);
                free(markov_node->data);
                free(markov_node);
            }
            free(current);
            current = next;
        }
        free(database);
    }
    free(markov_chain);
    *ptr_chain = NULL;
}
// Get a random starting node (not a sentence ender)
MarkovNode* get_first_random_node(MarkovChain *markov_chain) {
    if (markov_chain == NULL || markov_chain->database == NULL || 
        markov_chain->database->first == NULL) {
        return NULL;
    }
    
    // Count valid starting nodes (not sentence enders)
    Node *current = markov_chain->database->first;
    int valid_nodes_count = 0;
    
    while (current != NULL) {
        MarkovNode *markov_node = (MarkovNode*)current->data;
        if (markov_node != NULL && !is_sentence_ender(markov_node->data)) {
            valid_nodes_count++;
        }
        current = current->next;
    }
    
    if (valid_nodes_count == 0) {
        return NULL;
    }
    
    // Select random valid node
    int random_index = get_random_number(valid_nodes_count);
    current = markov_chain->database->first;
    int current_index = 0;
    
    while (current != NULL) {
        MarkovNode *markov_node = (MarkovNode*)current->data;
        if (markov_node != NULL && !is_sentence_ender(markov_node->data)) {
            if (current_index == random_index) {
                return markov_node;
            }
            current_index++;
        }
        current = current->next;
    }
    
    return NULL;
}

// Get the next random node based on frequency distribution
MarkovNode* get_next_random_node(MarkovNode *current_node) {
    if (current_node == NULL || current_node->frequency_list == NULL || 
        current_node->listLen == 0) {
        return NULL;
    }
    
    // Calculate total frequency
    int total_frequency = 0;
    for (int i = 0; i < current_node->listLen; i++) {
        total_frequency += current_node->frequency_list[i].frequency;
    }
    
    if (total_frequency == 0) {
        return NULL;
    }
    
    // Get random number in range [0, total_frequency)
    int random_val = get_random_number(total_frequency);
    
    // Use weighted random selection
    int cumulative = 0;
    for (int i = 0; i < current_node->listLen; i++) {
        cumulative += current_node->frequency_list[i].frequency;
        if (random_val < cumulative) {
            return current_node->frequency_list[i].markov_node;
        }
    }
    
    // Return last node if we somehow didn't select one
    return current_node->frequency_list[current_node->listLen - 1].markov_node;
}
