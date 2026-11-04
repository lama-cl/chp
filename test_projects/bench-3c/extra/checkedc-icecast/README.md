# Checked C port of icecast

This is a version of [icecast](https://icecast.org/) 2.44 that is being ported to [Checked C](https://github.com/microsoft/checkedc) with the help of [3C](https://github.com/correctcomputation/checkedc-clang/blob/main/clang/docs/checkedc/3C/README.md). Checked C is an extension of the C programming language that guarantees spatial memory safety (e.g., lack of buffer overflows) under certain assumptions.

For the original icecast readme, see the [README](README) file.

## Note about library headers

Icecast makes extensive use of external libraries written in C (libxml, libxslt, libogg, and libvorbis). To port the icecast code that interacts with those libraries to use Checked C safe pointers, we need versions of the library headers that are annotated with Checked C safe pointer types. In the future, we may have some scheme analogous to [DefinitelyTyped](https://definitelytyped.org/) to distribute Checked C headers for existing C libraries. But for now, we bundle our own versions of the headers in the `include` subdirectory of this repository, and we let 3C guess the annotations for the library APIs based on how they are used by icecast. See the [libjpeg tutorial](https://github.com/correctcomputation/checkedc-libjpeg-tutorial) for more information about this workflow and its caveats.

Our modified headers are based on the original headers for each library as packaged in Ubuntu 20.04. The license information for each set of headers (as provided by the respective Ubuntu package) is included in the `include/*/copyright` files.

## The porting process

The commits in the Git history of this repository represent steps of an incremental porting process, starting from an import of the original icecast source code. You can use the usual Git commands to browse the history (`git log`), check out previous stages of the port (`git checkout`), etc. At any point, 3C can be run via the `convert_all.sh` script (details in the next section), and it will write updated versions of the source files to a subdirectory named `out.checked`. Each commit in the history consists of changes made by 3C (i.e., we ran 3C and then copied some of the code from `out.checked` back to the original files), manual edits we made to port the code to Checked C, or a combination of both.

This port serves as an example of our recommended Checked C porting process, which has evolved a bit from what is described in the [the 3C usage documentation](https://github.com/correctcomputation/checkedc-clang/blob/main/clang/tools/3c/README.md) as of this writing (2021-12-08). We plan to make documentation of the new process available soon.

## Working with the port

You can build icecast, try to recreate previous porting steps we took (after checking out a previous commit), or improve the port further. Here's what you'll need to do:

1. [Build 3C](https://github.com/correctcomputation/checkedc-clang/blob/main/clang/docs/checkedc/3C/INSTALL.md) (including the `clang` target), but contrary to the normal instructions, use [Correct Computation's `checkedc` repository](https://github.com/correctcomputation/checkedc) because the port requires changes to the checked system headers that we have not yet had time to upstream. The port has last been tested with `checkedc-clang` commit 79804ee066f04e44583dd6df2dbe883b466aef5d and `checkedc` commit 19247d67f8c249eb1105c3ef05b49340e91458e8, so if you have trouble with the latest versions, try those versions. Add the `bin` directory of 3C to your `$PATH`.

2. Install [Bear](https://github.com/rizsotto/Bear).

3. Run the following to build icecast and generate `compile_commands.json` for use by 3C:

    ```
    ./configure --without-openssl CC=clang
    bear make -j $(nproc) -k
    ```

    `--without-openssl` is needed because the port was done without OpenSSL, so the OpenSSL-related code hasn't been ported and will generate compile errors. `CC=clang` uses the Checked C `clang` from step 1. `-j $(nproc)` does a parallel build and can be omitted if you prefer. `-k` tells `make` to attempt compilation of all source files even if one of them has an error, which ensures that all source files get added to `compile_commands.json` (though if we've done our job right, there shouldn't be any errors in the initial build).

4. To run a 3C pass on the whole project, run `./convert_all.sh`. This writes the updated versions of the source files to `out.checked`. Run `diff -ru . out.checked` to see what 3C changed; you can then copy chunks of code from the new files to the original files as desired. See [the 3C usage documentation](https://github.com/correctcomputation/checkedc-clang/blob/main/clang/tools/3c/README.md) for more information.

5. To perform an incremental build after you change the source files, run the same `make` command as in step 3, but _without_ `bear` (because the full set of source files and compiler options hasn't changed, and using `bear` on an incremental build would overwrite `compile_commands.json` to contain only the subset of files that changed in that build).

For a better experience working on Checked C code, consider using [Checked C's `clangd`](https://github.com/microsoft/checkedc-clang/blob/master/clang/docs/checkedc/clangd.md).
