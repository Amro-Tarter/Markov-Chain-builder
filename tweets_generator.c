#include "markov_chain.h"
#include "linked_list.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_PATH_ERROR "Error: incorrect file path"
#define MAX_TWEET_LENGTH 20
#define NUM_ARGS_ERROR "Usage: ./tweetsGenerator <seed> <tweet_count> <corpus_file> [words_to_read]"
#define DELIMITERS " \n\t\r"

// Function to fill the database from the file
int fill_database(FILE *file, int word_limit, MarkovChain *markov_chain) {
    int word_counter = 0;
    char line[1000];
    char delimiters[] = DELIMITERS;
    Node* prev_node = NULL;
    
    while (fgets(line, sizeof(line), file) != NULL && 
           (word_limit == 0 || word_counter < word_limit)) {
        char *word = strtok(line, delimiters);
        
        while (word != NULL && (word_limit == 0 || word_counter < word_limit)) {
            // Add current word to database
            Node *current_node = add_to_database(markov_chain, word);
            if (current_node == NULL) {
                return -1;
            }
            
            word_counter++;
            
            // Add to frequency list if we have a previous word
            if (prev_node != NULL) {
                if (add_node_to_frequency_list((MarkovNode*)prev_node->data, 
                                             (MarkovNode*)current_node->data) != 0) {
                    return -1;
                }
            }
            
            prev_node = current_node;
            word = strtok(NULL, delimiters);
            
            // Break if we've reached the word limit
            if (word_limit > 0 && word_counter >= word_limit) {
                break;
            }
        }
    }
    
    return word_counter;
}

// Function to print the Markov chain (for debugging)
void printChain(MarkovChain *markovChain) {
    if (!markovChain || !markovChain->database || !markovChain->database->first) {
        printf("Markov Chain or its database is empty.\n");
        return;
    }

    printf("Printing Markov Chain:\n");
    Node *current = markovChain->database->first;

    while (current != NULL) {
        MarkovNode *markov_node = (MarkovNode *)current->data;
        if (markov_node == NULL) {
            printf("Node has no data.\n");
            continue;
        }

        printf("Node: %s\n", markov_node->data);

        if (markov_node->listLen == 0) {
            printf("  Frequency list is empty.\n");
        } else {
            for (int i = 0; i < markov_node->listLen; ++i) {
                MarkovNodeFrequency *freqNode = &markov_node->frequency_list[i];
                if (freqNode && freqNode->markov_node) {
                    printf("  Frequency %d: Node = '%s', Frequency = %d\n",
                           i, freqNode->markov_node->data, freqNode->frequency);
                } else {
                    printf("  Frequency %d: NULL entry in frequency list.\n", i);
                }
            }
        }

        current = current->next;
    }

    printf("End of Markov Chain.\n");
}

// Function to generate tweets
void generate_tweets(MarkovChain *markov_chain, int tweet_count) {
    if (markov_chain == NULL || tweet_count <= 0) {
        return;
    }
    
    int generated_tweets = 0;
    while (generated_tweets < tweet_count) {
        MarkovNode *current_node = get_first_random_node(markov_chain);
        if (current_node == NULL) {
            continue;  // Try again if we can't get a starting node
        }
        
        printf("Tweet %d: %s", generated_tweets + 1, current_node->data);
        
        int words_in_tweet = 1;
        bool ended = false;
        
        while (!ended && words_in_tweet < MAX_TWEET_LENGTH) {
            MarkovNode *next_node = get_next_random_node(current_node);
            if (next_node == NULL) {
                break;
            }
            
            printf(" %s", next_node->data);
            words_in_tweet++;
            
            if (is_sentence_ender(next_node->data)) {
                ended = true;
            } else {
                current_node = next_node;
            }
        }
        
        if (!ended) {
            // Find a sentence ender to conclude the tweet
            Node *current = markov_chain->database->first;
            while (current != NULL) {
                MarkovNode *node = (MarkovNode*)current->data;
                if (is_sentence_ender(node->data)) {
                    printf(" %s", node->data);
                    break;
                }
                current = current->next;
            }
        }
        
        printf("\n");
        generated_tweets++;
    }
}
int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "%s\n", NUM_ARGS_ERROR);
        return EXIT_FAILURE;
    }

    int seed = atoi(argv[1]);
    srand(seed);

    int tweet_count = atoi(argv[2]);
    char *path = argv[3];

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "%s\n", FILE_PATH_ERROR);
        return EXIT_FAILURE;
    }

    MarkovChain *markov_chain = (MarkovChain *)malloc(sizeof(MarkovChain));
    if (markov_chain == NULL) {
        fprintf(stderr, "%s\n", ALLOCATION_ERROR_MASSAGE);
        fclose(file);
        return EXIT_FAILURE;
    }

    markov_chain->database = (LinkedList *)malloc(sizeof(LinkedList));
    if (markov_chain->database == NULL) {
        fprintf(stderr, "%s\n", ALLOCATION_ERROR_MASSAGE);
        free(markov_chain);
        fclose(file);
        return EXIT_FAILURE;
    }

    markov_chain->database->first = NULL;
    markov_chain->database->last = NULL;
    markov_chain->database->size = 0;

    int word_limit = (argc == 5) ? atoi(argv[4]) : 0;
    if (fill_database(file, word_limit, markov_chain) == -1) {
        fclose(file);
        free_database(&markov_chain);
        return EXIT_FAILURE;
    }

    fclose(file);


    // Generate tweets
    generate_tweets(markov_chain, tweet_count);

    // Free memory
    free_database(&markov_chain);

    return EXIT_SUCCESS;
}
