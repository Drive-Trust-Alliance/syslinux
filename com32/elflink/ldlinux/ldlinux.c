#include <linux/list.h>
#include <sys/times.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <core.h>
#include <fs.h>
#include "cli.h"
#include "console.h"
#include "com32.h"
#include "menu.h"
#include "config.h"
#include "syslinux/adv.h"
#include "syslinux/boot.h"

#include <sys/module.h>

struct file_ext {
	const char *name;
	enum kernel_type type;
};

static const struct file_ext file_extensions[] = {
	{ ".com", IMAGE_TYPE_COMBOOT },
	{ ".cbt", IMAGE_TYPE_COMBOOT },
	{ ".c32", IMAGE_TYPE_COM32 },
	{ ".img", IMAGE_TYPE_FDIMAGE },
	{ ".bss", IMAGE_TYPE_BSS },
	{ ".bin", IMAGE_TYPE_BOOT },
	{ ".bs", IMAGE_TYPE_BOOT },
	{ ".0", IMAGE_TYPE_PXE },
	{ NULL, 0 },
};

/*
 * Return a pointer to one byte after the last character of the
 * command.
 */
static inline const char *find_command(const char *str)
{
	const char *p;

	p = str;
	while (*p && !my_isspace(*p))
		p++;
	return p;
}

uint32_t parse_image_type(const char *kernel)
{
	const struct file_ext *ext;
	const char *p;
	int len;

	/* Find the end of the command */
	p = find_command(kernel);
	len = p - kernel;

	for (ext = file_extensions; ext->name; ext++) {
		int elen = strlen(ext->name);

		if (!strncmp(kernel + len - elen, ext->name, elen))
			return ext->type;
	}

	/* use IMAGE_TYPE_KERNEL as default */
	return IMAGE_TYPE_KERNEL;
}

/*
 * Returns the kernel name with file extension if one wasn't present.
 */
static const char *get_extension(const char *kernel)
{
	const struct file_ext *ext;
	const char *p;
	int len;

	/* Find the end of the command */
	p = find_command(kernel);
	len = p - kernel;

	for (ext = file_extensions; ext->name; ext++) {
		char *str;
		int elen = strlen(ext->name);
		FILE *f;

		str = malloc(len + elen + 1);

		strncpy(str, kernel, len);
		strncpy(str + len, ext->name, elen);
		str[len + elen] = '\0';
		f = findpath(str);
		free(str);

		if (f) {
			fclose(f);
			return ext->name;
		}
	}

	return NULL;
}

static const char *apply_extension(const char *kernel, const char *ext)
{
	const char *p;
	char *k;
	int len = strlen(kernel);
	int elen = strlen(ext);

	k = malloc(len + elen + 1);
	if (!k)
		return NULL;

	p = find_command(kernel);

	len = p - kernel;

	/* Copy just the kernel name */
	memcpy(k, kernel, len);

	/* Append the extension */
	memcpy(k + len, ext, elen);

	/* Copy the rest of the command line */
	strcpy(k + len + elen, p);

	k[len + elen + strlen(p)] = '\0';

	return k;
}

/*
 * Attempt to load a kernel after deciding what type of image it is.
 *
 * We only return from this function if something went wrong loading
 * the the kernel. If we return the caller should call enter_cmdline()
 * so that the user can help us out.
 */
void load_kernel(const char *command_line)
{
	struct menu_entry *me;
	const char *cmdline;
	const char *kernel;
	uint32_t type;

	kernel = strdup(command_line);
	if (!kernel)
		goto bad_kernel;

	/* Virtual kernel? */
	me = find_label(kernel);
	if (me) {
		type = parse_image_type(me->cmdline);

		execute(me->cmdline, type);
		/* We shouldn't return */
		goto bad_kernel;
	}

	if (!allowimplicit)
		goto bad_implicit;

	/* Insert a null character to ignore any user-specified options */
	if (!allowoptions) {
		char *p = (char *)find_command(kernel);
		*p = '\0';
	}

	type = parse_image_type(kernel);
	if (type == IMAGE_TYPE_KERNEL) {
		const char *ext;

		/*
		 * Automatically lookup the extension if one wasn't
		 * supplied by the user.
		 */
		ext = get_extension(kernel);
		if (ext) {
			const char *k;

			k = apply_extension(kernel, ext);
			if (!k)
				goto bad_kernel;

			free((void *)kernel);
			kernel = k;

			type = parse_image_type(kernel);
		}
	}

	execute(kernel, type);
	free((void *)kernel);

bad_implicit:
bad_kernel:
	/*
	 * If we fail to boot the kernel execute the "onerror" command
	 * line.
	 */
	if (onerrorlen) {
		rsprintf(&cmdline, "%s %s", onerror, default_cmd);
		execute(cmdline, IMAGE_TYPE_COM32);
	}
}

static void enter_cmdline(void)
{
	const char *cmdline;

	/* Enter endless command line prompt, should support "exit" */
	while (1) {
		cmdline = edit_cmdline("boot:", 1, NULL, cat_help_file);
		printf("\n");

		/* return if user only press enter or we timed out */
		if (!cmdline || cmdline[0] == '\0')
			return;

		load_kernel(cmdline);
	}
}

void ldlinux_enter_command(bool prompt)
{
	const char *cmdline = default_cmd;

	if (prompt)
		goto cmdline;
auto_boot:
	/*
	 * Auto boot
	 */
	if (defaultlevel || noescape) {
		if (defaultlevel) {
			load_kernel(cmdline); /* Shouldn't return */
		} else {
			printf("No DEFAULT or UI configuration directive found!\n");

			if (noescape)
				kaboom();
		}
	}

cmdline:
	/* Only returns if the user pressed enter or input timed out */
	enter_cmdline();

	cmdline = ontimeoutlen ? ontimeout : default_cmd;

	goto auto_boot;
}

/*
 * Undo the work we did in openconsole().
 */
static void __destructor close_console(void)
{
	int i;

	for (i = 0; i <= 2; i++)
		close(i);
}

int main(int argc __unused, char **argv __unused)
{
	const void *adv;
	const char *cmdline;
	size_t count = 0;
	char *config_argv[2] = { NULL, NULL };

	openconsole(&dev_rawcon_r, &dev_ansiserial_w);

	if (ConfigName[0])
		config_argv[0] = ConfigName;

	parse_configs(config_argv);

	adv = syslinux_getadv(ADV_BOOTONCE, &count);
	if (adv && count) {
		/*
		 * We apparently have a boot-once set; clear it and
		 * then execute the boot-once.
		 */
		char *src, *dst;
		size_t i;

		src = (char *)adv;
		cmdline = dst = malloc(count + 1);
		if (!dst) {
			printf("Failed to allocate memory for ADV\n");
			ldlinux_enter_command(true);
		}

		for (i = 0; i < count; i++)
			*dst++ = *src++;
		*dst = '\0';	/* Null-terminate */

		/* Clear the boot-once data from the ADV */
		if (!syslinux_setadv(ADV_BOOTONCE, 0, NULL))
			syslinux_adv_write();

		load_kernel(cmdline); /* Shouldn't return */
		ldlinux_enter_command(true);
	}

	/* TODO: Check KbdFlags? */

	ldlinux_enter_command(forceprompt);
	return 0;
}