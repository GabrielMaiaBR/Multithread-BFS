#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>

// Definir limite de vértices
#define MAX 102500

int **graph;
bool *visited;
int num_vertices;
pthread_mutex_t mutex;
pthread_cond_t cond;
int active_threads = 0;
int max_threads = 8;

typedef struct {
    int vertex;
} ThreadData;

void* bfs_threaded(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int v = data->vertex;
    free(data);

    for (int i = 0; i < num_vertices; i++) {
        pthread_mutex_lock(&mutex);
        if (graph[v][i] && !visited[i]) {
            visited[i] = true;
            if (active_threads < max_threads) {
                active_threads++;
                ThreadData* new_data = (ThreadData*)malloc(sizeof(ThreadData));
                if (new_data == NULL) {
                    fprintf(stderr, "Erro ao alocar memória para ThreadData\n");
                    exit(EXIT_FAILURE);
                }
                new_data->vertex = i;
                pthread_t new_thread;
                pthread_create(&new_thread, NULL, bfs_threaded, new_data);
                pthread_detach(new_thread);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    active_threads--;
    if (active_threads == 0) {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void bfs_single_thread(int start_vertex) {
    int *queue = (int *)malloc(num_vertices * sizeof(int));
    if (queue == NULL) {
        fprintf(stderr, "Erro ao alocar memória para a fila\n");
        exit(EXIT_FAILURE);
    }

    int front = 0, rear = 0;

    visited[start_vertex] = true;
    queue[rear++] = start_vertex;

    while (front < rear) {
        int current_vertex = queue[front++];

        for (int i = 0; i < num_vertices; i++) {
            if (graph[current_vertex][i] && !visited[i]) {
                visited[i] = true;
                queue[rear++] = i;
            }
        }
    }

    free(queue);
}

void initialize_graph(int num_vertices) {
    graph = (int **)malloc(num_vertices * sizeof(int *));
    if (graph == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o grafo\n");
        exit(EXIT_FAILURE);
    }

    visited = (bool *)malloc(num_vertices * sizeof(bool));
    if (visited == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o array de visitados\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_vertices; i++) {
        graph[i] = (int *)malloc(num_vertices * sizeof(int));
        if (graph[i] == NULL) {
            fprintf(stderr, "Erro ao alocar memória para a linha do grafo\n");
            exit(EXIT_FAILURE);
        }
        visited[i] = false;
        for (int j = 0; j < num_vertices; j++) {
            graph[i][j] = 0;
        }
    }

    for (int i = 0; i < num_vertices; i++) {
        if (i + 1 < num_vertices) {
            graph[i][i + 1] = 1;
        }
        if (i + 2 < num_vertices) {
            graph[i][i + 2] = 1;
        }
    }
}

void reset_visited() {
    for (int i = 0; i < num_vertices; i++) {
        visited[i] = false;
    }
}

void free_graph() {
    for (int i = 0; i < num_vertices; i++) {
        free(graph[i]);
    }
    free(graph);
    free(visited);
}

int main() {
    FILE *file = fopen("bfs_comparison_results.csv", "w");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "Vertices,BFS_without_threads,BFS_with_threads\n");

    for (num_vertices = 2; num_vertices <= 10000; num_vertices *= 2) {
        initialize_graph(num_vertices);

        struct timeval start, end;
        long microseconds_no_threads, microseconds_with_threads;

        // BFS sem Multithreading
        gettimeofday(&start, NULL);

        bfs_single_thread(0);

        gettimeofday(&end, NULL);
        long seconds = (end.tv_sec - start.tv_sec);
        microseconds_no_threads = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        reset_visited();

        // BFS com Multithreading
        gettimeofday(&start, NULL);

        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        visited[0] = true;
        active_threads++;

        ThreadData* data = (ThreadData*)malloc(sizeof(ThreadData));
        if (data == NULL) {
            fprintf(stderr, "Erro ao alocar memória para ThreadData\n");
            exit(EXIT_FAILURE);
        }
        data->vertex = 0;
        pthread_t thread;
        pthread_create(&thread, NULL, bfs_threaded, data);
        
        pthread_mutex_lock(&mutex);
        while (active_threads > 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);

        gettimeofday(&end, NULL);
        seconds = (end.tv_sec - start.tv_sec);
        microseconds_with_threads = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        fprintf(file, "%d,%ld,%ld\n", num_vertices, microseconds_no_threads, microseconds_with_threads);

        reset_visited();
        free_graph();
    }

    fclose(file);

    return 0;
}
