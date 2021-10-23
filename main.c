#include <stdbool.h>
/* This package allows to work with booleans */
#include <stdio.h>
/* This package allows to work with input and output functions like fprint or fopen */
#include <stdint.h>
/* This package allows to work with integers */
#include <stdlib.h>
/* This package is for general use of c language */
#include <string.h>
/* This package allows to work with strings */

/* This package looks like error, but works in linux like in windows */
#include <dirent.h>
/* This package allows to work with directories */

/* Variable declaration for buffers */
#define CMP_BUFF_TYPE uint8_t
#define CMP_BUFF_SIZE 1024
#define FILEPATH_BUFF_SIZE 512

/* Variable declaration for input and output directories */
#define DIR_DATA_PROOF "data_proof"
#define DIR_DATA_UNIQUE "data_unique"

/* As its name says, it is to separate directories */
#define DIR_SEPARATOR "/"

typedef struct node
{
  char filename[FILENAME_MAX];
  struct node *next;
} node_t;

node_t *node_alloc(const char *filename)
{
  node_t *node = malloc(sizeof(node_t));
  strncpy(node->filename, filename, FILENAME_MAX);
  node->next = NULL;
  return node;
}

void node_free(node_t *node)
{
  node_t *curr_node = node;
  node_t *next_node;

  while (curr_node != NULL)
  {
    next_node = curr_node->next;
    free(curr_node);
    curr_node = next_node;
  }
}

void node_add(node_t *root, const char *filename)
{
  node_t *curr_node = root;

  while (curr_node->next != NULL)
  {
    curr_node = curr_node->next;
  }

  node_t *node = node_alloc(filename);
  curr_node->next = node;
}

void node_remove(node_t *root, node_t *node)
{
  if (node == NULL)
  {
    return;
  }

  node_t *curr_node = root;
  node_t *next_node;

  while (curr_node != NULL)
  {
    if (curr_node->next == node)
    {
      next_node = node->next;
      free(curr_node->next);
      curr_node->next = next_node;
    }

    curr_node = curr_node->next;
  }
}

int compare_fstreams(const char *first_filepath, const char *second_filepath)
{
  FILE *first_fp = fopen(first_filepath, "rb");
  FILE *second_fp = fopen(second_filepath, "rb");

  CMP_BUFF_TYPE first_buff[CMP_BUFF_SIZE];
  CMP_BUFF_TYPE second_buff[CMP_BUFF_SIZE];

  int result = 0;
  while (true)
  {
    size_t first_size = fread(first_buff, sizeof(CMP_BUFF_TYPE), CMP_BUFF_SIZE, first_fp);
    size_t second_size = fread(second_buff, sizeof(CMP_BUFF_TYPE), CMP_BUFF_SIZE, second_fp);

    if (first_size == second_size)
    {
      int cmp = memcmp(first_buff, second_buff, first_size);
      if (cmp != 0)
      {
        result = cmp;
        break;
      }
    }

    if (first_size != CMP_BUFF_SIZE || second_size != CMP_BUFF_SIZE)
    {
      if (feof(first_fp) && feof(second_fp))
      {
        break;
      }
      else if (feof(first_fp))
      {
        result = -1;
        break;
      }
      else if (feof(second_fp))
      {
        result = 1;
        break;
      }

      if (ferror(first_fp) || ferror(second_fp))
      {
        perror(first_filepath);
        perror(second_filepath);
        perror("An error ocurred when comparing both files");
        break;
      }
    }
  }

  fclose(first_fp);
  fclose(second_fp);

  return result;
}

void store_filepath(char *dst, const char *dir, const char *filename, size_t size)
{
  strncat(dst, dir, size);
  strncat(dst, DIR_SEPARATOR, size);
  strncat(dst, filename, size);
}

node_t *directory_to_node(const char *dirname)
{
  node_t *root = NULL;

  DIR *dir = opendir(dirname);
  if (dir == NULL)
  {
    perror("Something is wrong opening dir");
  }

  struct dirent *entry;
  char filepath[FILENAME_MAX];

  puts("Creating file list");
  while ((entry = readdir(dir)) != NULL)
  {
    if (strncmp(entry->d_name, ".", FILENAME_MAX) == 0)
    {
      continue;
    }
    if (strncmp(entry->d_name, "..", FILENAME_MAX) == 0)
    {
      continue;
    }

    memset(filepath, '\0', FILENAME_MAX);
    store_filepath(filepath, dirname, entry->d_name, FILENAME_MAX);

    if (root == NULL)
    {
      root = node_alloc(filepath);
    }
    else
    {
      node_add(root, filepath);
    }
  }

  closedir(dir);

  return root;
}

void remove_duplicates(node_t *root)
{
  node_t *curr_node = root;
  node_t *next_node;
  node_t *dup_node;

  int is_equal = 0;

  puts("Removing duplicates");
  while (curr_node != NULL)
  {
    printf("Processing %s\n", curr_node->filename);

    next_node = curr_node->next;
    while (next_node != NULL)
    {
      // printf("%s vs %s\n", curr_node->filename, next_node->filename);

      is_equal = compare_fstreams(curr_node->filename, next_node->filename);
      if (is_equal == 0)
      {
        dup_node = next_node;
        next_node = next_node->next;
        node_remove(root, dup_node);
      }
      else
      {
        next_node = next_node->next;
      }
    }

    curr_node = curr_node->next;
  }
}

void copy_files(node_t *root, const char *dstdir)
{
  node_t *curr_node = root;

  FILE *read_stream;
  FILE *write_stream;

  char dst_filepath[FILENAME_MAX];
  char *filename;

  int c;

  puts("Copying unique files");
  while (curr_node != NULL)
  {
    memset(dst_filepath, '\0', FILENAME_MAX);
    filename = strchr(curr_node->filename, DIR_SEPARATOR[0]);
    store_filepath(dst_filepath, dstdir, filename, FILENAME_MAX);

    read_stream = fopen(curr_node->filename, "rb");
    if (read_stream == NULL) {
      perror("Error reading file");
      perror(curr_node->filename);
      break;
    }

    write_stream = fopen(dst_filepath, "wb");
    if (write_stream == NULL) {
      fclose(read_stream);
      perror("Error writing file");
      perror(dst_filepath);
      break;
    }

    while ((c = fgetc(read_stream)) != EOF)
    {
      fputc(c, write_stream);
    }

    fclose(read_stream);
    fclose(write_stream);

    curr_node = curr_node->next;
  }
}

int main(int argc, char **argv)
{
  node_t *root = directory_to_node(DIR_DATA_PROOF);
  if (root == NULL)
  {
    return 1;
  }

  remove_duplicates(root);

  copy_files(root, DIR_DATA_UNIQUE);

  int files = 0;
  node_t *curr_node = root;
  while (curr_node != NULL)
  {
    files++;
    curr_node = curr_node->next;
  }

  printf("There are %d unique files\n", files);

  node_free(root);

  return 0;
}
