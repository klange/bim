/**
 * Bim 3.0
 *
 * This is essentially a complete re-write of Bim.
 */
#include <locale.h>

#include "kuroko/src/kuroko.h"
#include "kuroko/src/object.h"
#include "kuroko/src/vm.h"

#include "buffer.h"
#include "util.h"
#include "view.h"
#include "input.h"

extern void kuroko_bind_syntax(KrkInstance * module);
extern void kuroko_bind_buffer(KrkInstance * module);
extern void kuroko_bind_view(KrkInstance * module);
extern KrkValue kuroko_TextBuffer_from_buffer(buffer_t * env);

extern void view_test(buffer_t * buffer);

void set_alternate_screen(void) {
	printf("\033[?1049h");
}

/**
 * Restore the standard terminal screen.
 */
void unset_alternate_screen(void) {
	printf("\033[?1049l");
}

int main(int argc, char * argv[]) {
	setlocale(LC_ALL, "");
	krk_initVM(0);

	KrkInstance * bimModule = krk_newInstance(vm.moduleClass);
	krk_attachNamedObject(&vm.modules, "bim", (KrkObj*)bimModule);
	krk_attachNamedObject(&bimModule->fields, "__name__", (KrkObj*)S("bim"));

	kuroko_bind_buffer(bimModule);
	kuroko_bind_syntax(bimModule);
	kuroko_bind_view(bimModule);

	krk_startModule("<bim-repl>");

	buffer_t * commandBuffer = buffer_new();
	commandBuffer->numbers = 0;
	krk_attachNamedValue(&vm.module->fields, "commandBuffer", kuroko_TextBuffer_from_buffer(commandBuffer));
	struct TextView * commandView = view_from_buffer(commandBuffer);
	commandView->y = commandView->h;
	commandView->h = 1;
	krk_attachNamedObject(&vm.builtins->fields, "commandline", (KrkObj*)commandView);

	buffer_t * myFile = buffer_from_file("bim._c");
	krk_attachNamedValue(&vm.module->fields, "env", kuroko_TextBuffer_from_buffer(myFile));

	krk_resetStack();
	krk_interpret(
		"import bim\n"
		"import syntax.c\n"
		"bim.xxx_highlight_buffer(env)\n"
	,0,"<bim>","<bim>");

	struct TextView * myView = view_from_buffer(myFile);

	krk_attachNamedObject(&vm.builtins->fields, "view", (KrkObj*)myView);

	krk_resetStack();

#if 0
	input_initialize();
	set_alternate_screen();
	fflush(stdout);

	while (1) {
		view_redraw(myView);
		int key = input_getkey(-1);

		if (key == KEY_UP && myView->offset > 0) {
			myView->offset--;
		} else if (key == KEY_DOWN && myView->offset < myView->env->line_count - 1) {
			myView->offset++;
		}

		if (key == 'q') break;
	}

	unset_alternate_screen();
	input_release();
#endif

	while (1) {
		char buf[4096];
		fprintf(stdout, ">>> ");
		fflush(stdout);
		fgets(buf, 4096, stdin);
		if (feof(stdin)) break;
		/* Call the interpreter */
		KrkValue out = krk_interpret(buf,0,"<bim>","<bim>");
		if (!IS_NONE(out)) {
			krk_push(out);
			KrkValue repr = krk_callSimple(OBJECT_VAL(krk_getType(out)->_reprer), 1, 0);
			if (IS_STRING(repr)) {
				fprintf(stdout, " => %s\n", AS_CSTRING(repr));
			}
		}
		krk_resetStack();
	}

	krk_freeVM();
	return 0;
}
