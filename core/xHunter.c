/* 
 * JavaScript Processor (jspc).
 * Generate the parse-tree of a JavaScript snippet.
 * Elias Athanasopoulos  <elathan@ics.forth.gr>
 * Februrary 2010.
 * 
 * Heavily based on: http://siliconforks.com/doc/parsing-javascript-with-spidermonkey/
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <js/jsapi.h>
#include <js/jsparse.h>
#include <js/jsscan.h>
#include <js/jsstr.h>

JSRuntime * runtime;
JSContext * context;
JSObject * global;
unsigned int xhunter_depth = 0;


void xhunter_init(void)
{
  runtime = JS_NewRuntime(8L * 1024L * 1024L);
  if (runtime == NULL) {
    fprintf(stderr, "Cannot create runtime.");
    exit(EXIT_FAILURE);
  }

  context = JS_NewContext(runtime, 8192);
  if (context == NULL) {
    fprintf(stderr, "Cannot create context.");
    exit(EXIT_FAILURE);
  }

  global = JS_NewObject(context, NULL, NULL, NULL);
  if (global == NULL) {
    fprintf(stderr, "Cannot create global object/");
    exit(EXIT_FAILURE);
  }

  if (!JS_InitStandardClasses(context, global)) {
    fprintf(stderr, "Cannot initialize standard classes.");
    exit(EXIT_FAILURE);
  }  
  
}

void xhunter_finalize(void)
{
  JS_DestroyContext(context);
  JS_DestroyRuntime(runtime);  
}

const char * TOKENS[81] = {
  "EOF", "EOL", "SEMI", "COMMA", "ASSIGN", "HOOK", "COLON", "OR", "AND",
  "BITOR", "BITXOR", "BITAND", "EQOP", "RELOP", "SHOP", "PLUS", "MINUS", "STAR",
  "DIVOP", "UNARYOP", "INC", "DEC", "DOT", "LB", "RB", "LC", "RC", "LP", "RP",
  "NAME", "NUMBER", "STRING", "OBJECT", "PRIMARY", "FUNCTION", "EXPORT",
  "IMPORT", "IF", "ELSE", "SWITCH", "CASE", "DEFAULT", "WHILE", "DO", "FOR",
  "BREAK", "CONTINUE", "IN", "VAR", "WITH", "RETURN", "NEW", "DELETE",
  "DEFSHARP", "USESHARP", "TRY", "CATCH", "FINALLY", "THROW", "INSTANCEOF",
  "DEBUGGER", "XMLSTAGO", "XMLETAGO", "XMLPTAGC", "XMLTAGC", "XMLNAME",
  "XMLATTR", "XMLSPACE", "XMLTEXT", "XMLCOMMENT", "XMLCDATA", "XMLPI", "AT",
  "DBLCOLON", "ANYNAME", "DBLDOT", "FILTER", "XMLELEM", "XMLLIST", "RESERVED",
  "LIMIT",
};

const int NUM_TOKENS = sizeof(TOKENS) / sizeof(TOKENS[0]);

int xhunter_filter_node(const char *node)
{
  if (!strncmp(node, "DOT", strlen("DOT")) ||
      !strncmp(node, "PLUS", strlen("PLUS")) ||
      !strncmp(node, "DIVOP", strlen("DIVOP")) ||
      !strncmp(node, "MINUS", strlen("MINUS")) ||
      !strncmp(node, "STAR", strlen("STAR")) ||
      !strncmp(node, "BITOR", strlen("BITOR")) ||
      !strncmp(node, "XML", strlen("XML"))
      ) return 1;
  
  return 0;
}

int xhunter_weight_node(const char *node)
{
  if (!strncmp(node, "LP", strlen("LP"))) 
    return 2;
  if (!strncmp(node, "NAME", strlen("NAME"))) 
    return -1;
  
  return 0;
}

void xhunter_print_tree(JSParseNode * root, int indent, int show_output) 
{    
  if (root == NULL) {
    return;
  }
  
  xhunter_depth++;

  if (xhunter_filter_node(TOKENS[root->pn_type])) {
    indent--;
    xhunter_depth--;
  }
  
  xhunter_depth += xhunter_weight_node(TOKENS[root->pn_type]);

  if (show_output) {
    if (root->pn_type >= NUM_TOKENS) {
      printf("UNKNOWN");
    }
    else {  
      printf("%s ", TOKENS[root->pn_type]);
      // printf("%s", TOKENS[root->pn_type]);   
      // printf("%*s", indent, "");
      //       printf("%s: starts at line %d, column %d, ends at line %d, column %d",
      //                TOKENS[root->pn_type],
      //                root->pn_pos.begin.lineno, root->pn_pos.begin.index,
      //                root->pn_pos.end.lineno, root->pn_pos.end.index);
      }
      // printf("\n");
  }

  switch (root->pn_arity) {
  case PN_UNARY:
    xhunter_print_tree(root->pn_kid, indent + 1, show_output);
    break;
  case PN_BINARY:
    if (root->pn_left != root->pn_right)
      xhunter_print_tree(root->pn_left, indent + 1, show_output);
    xhunter_print_tree(root->pn_right, indent + 1, show_output);
    break;
  case PN_TERNARY:
    xhunter_print_tree(root->pn_kid1, indent + 1, show_output);
    xhunter_print_tree(root->pn_kid2, indent + 1, show_output);
    xhunter_print_tree(root->pn_kid3, indent + 1, show_output);
    break;
  case PN_LIST:
    {
      JSParseNode * p;
      for (p = root->pn_head; p != NULL; p = p->pn_next) {
        xhunter_print_tree(p, indent + 1, show_output);
      }
    }
    break;
  case PN_FUNC:
    xhunter_print_tree(root->pn_body, indent + 1, show_output);
    break;
  case PN_NAME:
    xhunter_print_tree(root->pn_expr, indent + 1, show_output);
    break;
  case PN_NULLARY:  
    break;
  default:
    fprintf(stderr, "Unknown node type\n");
    exit(EXIT_FAILURE);
    break;
  }
}

int xhunter_parse_string(const char *code, int show_output)
{
  JSParseContext pc;
  JSParseNode * node;
  jschar *source;
  size_t length = strlen(code);

  /* Initialize global structures.  */
  xhunter_init();

  source = js_InflateString(context, code, &length);
    
  if (!js_InitParseContext(context, &pc, NULL, source, length, NULL, NULL, 0)) 
   exit(EXIT_FAILURE); 

  node = js_ParseScript(context, global, &pc);
  if (node == NULL) {
    JS_free(context, source);
    xhunter_finalize();
    return -1;
  }

  xhunter_depth = 0;
  xhunter_print_tree(node, 0, show_output);
  
  JS_free(context, source);
  xhunter_finalize();
  
  return xhunter_depth;
}

// 
// int main(int argc, char **argv)
// {
//   xhunter_parse_string("var x = 1; print(/Hello World/);");
//   return 1;
// }
