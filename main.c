// para compilar a biblioteca utilizar o comando abaixo
// gcc main.c sqlite3.c -o biblioteca.exe

// para compilar o codigo utilzamos o comando abaixo
// gcc main.c sqlite3.c mongoose.c -o biblioteca.exe -D_WIN32_WINNT=0x0600 -lws2_32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "mongoose.h" 

// --- CORREÇÃO DEFINITIVA PARA O WINDOWS ---
int fseeko(FILE *stream, long offset, int whence) {
    return fseek(stream, offset, whence);
}
// ------------------------------------------

/* =========================================
   1. MODELAGEM DOS DADOS
   ========================================= */
typedef struct {
    int id;
    char title[150];
    char author[100];
    int year;
    int status;
} Book;

sqlite3 *db; 

/* =========================================
   2. BANCO DE DADOS E LÓGICA
   ========================================= */
sqlite3* init_database(const char* db_name) {
    sqlite3 *local_db;
    sqlite3_open(db_name, &local_db);
    const char *sql_create = 
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, author TEXT, year INTEGER, status INTEGER DEFAULT 1);";
    sqlite3_exec(local_db, sql_create, 0, 0, 0);
    return local_db;
}

// NOVO: Função Seeder para popular o banco de dados automaticamente
void seed_database(sqlite3 *local_db) {
    sqlite3_stmt *stmt;
    int count = 0;
    
    // Verifica se já existem livros na tabela
    sqlite3_prepare_v2(local_db, "SELECT COUNT(*) FROM books;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    // Se estiver vazio, insere os nossos 3 livros de teste
    if (count == 0) {
        const char *sql_insert = 
            "INSERT INTO books (id, title, author, year, status) VALUES "
            "(1, 'Linguagem C: Completa e Descomplicada', 'André Backes', 2012, 1),"
            "(2, 'Código Limpo', 'Robert C. Martin', 2008, 1),"
            "(3, 'O Programador Pragmático', 'Andrew Hunt', 1999, 1);";
        sqlite3_exec(local_db, sql_insert, 0, 0, 0);
        printf(">> SEEDER: Banco de dados populado com 3 livros iniciais!\n");
    }
}

int donate_book(sqlite3 *db, int book_id) {
    sqlite3_stmt *stmt;
    const char *sql_update = "UPDATE books SET status = 0 WHERE id = ? AND status = 1;";
    sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, book_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (sqlite3_changes(db) > 0) ? 1 : 0;
}

/* =========================================
   3. SERVIDOR HTTP / API REST
   ========================================= */
static void api_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        const char *cors_headers = "Access-Control-Allow-Origin: *\r\n"
                                   "Access-Control-Allow-Methods: GET, DELETE, OPTIONS\r\n"
                                   "Content-Type: application/json\r\n";

        if (strncmp(hm->uri.buf, "/api/livros", 11) == 0) {
            if (strncmp(hm->method.buf, "OPTIONS", 7) == 0) {
                mg_http_reply(c, 200, cors_headers, "");
            }
            else if (strncmp(hm->method.buf, "DELETE", 6) == 0) {
                int id = 0;
                if (hm->query.len > 0) {
                    char query_str[50] = {0};
                    snprintf(query_str, sizeof(query_str), "%.*s", (int)hm->query.len, hm->query.buf);
                    sscanf(query_str, "id=%d", &id);
                }

                if (id > 0 && donate_book(db, id)) {
                    printf(">> API: Livro ID %d doado com sucesso!\n", id);
                    mg_http_reply(c, 200, cors_headers, "{\"sucesso\": true, \"mensagem\": \"Livro marcado como doado no banco!\"}");
                } else {
                    printf(">> API: Falha ao doar livro ID %d.\n", id);
                    mg_http_reply(c, 400, cors_headers, "{\"sucesso\": false, \"mensagem\": \"Erro ou inexistente\"}");
                }
            } else {
                mg_http_reply(c, 405, cors_headers, "{\"erro\": \"Metodo nao permitido\"}");
            }
        } else {
            mg_http_reply(c, 404, cors_headers, "{\"erro\": \"Rota nao encontrada\"}");
        }
    }
}

int main() {
    db = init_database("biblioteca.db");
    
    // Chama o seeder para garantir que temos dados!
    seed_database(db);
    
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://localhost:8000", api_handler, NULL);

    printf("Servidor C rodando! Aguardando o Front-end em http://localhost:8000\n");
    printf("Pressione Ctrl+C para encerrar.\n\n");

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    sqlite3_close(db);
    return 0;
}
//se houver erro na compilação
//gcc main.c sqlite3.c mongoose.c -o biblioteca.exe -D_WIN32_WINNT=0x0600 -Dfseeko=fseek -lws2_32
//ou ainda o codigo abaixo
//gcc main.c sqlite3.c mongoose.c -o biblioteca.exe -D_WIN32_WINNT=0x0600 -lws2_32
//.\biblioteca.exe