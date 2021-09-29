
local bsdiff = require("bsdiff")

local old_content = [[
    bsdiff and bspatch are tools for building and applying patches to binary files. By using suffix sorting (specifically, Larsson and Sadakane's qsufsort) and taking advantage of how executable files change, bsdiff routinely produces binary patches 50-80% smaller than those produced by Xdelta, and 15% smaller than those produced by .RTPatch (a $2750/seat commercial patch tool).
    These programs were originally named bdiff and bpatch, but the large number of other programs using those names lead to confusion; I'm not sure if the "bs" in refers to "binary software" (because bsdiff produces exceptionally small patches for executable files) or "bytewise subtraction" (which is the key to how well it performs). Feel free to offer other suggestions.

    bsdiff and bspatch use bzip2; by default they assume it is in /usr/bin.

    bsdiff is quite memory-hungry. It requires max(17*n,9*n+m)+O(1) bytes of memory, where n is the size of the old file and m is the size of the new file. bspatch requires n+m+O(1) bytes.

    bsdiff runs in O((n+m) log n) time; on a 200MHz Pentium Pro, building a binary patch for a 4MB file takes about 90 seconds. bspatch runs in O(n+m) time; on the same machine, applying that patch takes about two seconds.

    Providing that off_t is defined properly, bsdiff and bspatch support files of up to 2^61-1 = 2Ei-1 bytes.

    Version 4.3 is available here with MD5 hash e6d812394f0e0ecc8d5df255aa1db22a. Version 4.2 is available in the FreeBSD, NetBSD, and OpenBSD ports trees as misc/bsdiff, in Darwinports as devel/bsdiff, and in gentoo as dev-util/bsdiff. It has also been made into a Python extension module.

    The algorithm used by BSDiff 4 is described in my (unpublished) paper Naive differences of executable code; please cite this in papers as

    Colin Percival, Naive differences of executable code, http://www.daemonology.net/bsdiff/, 2003.
    A far more sophisticated algorithm, which typically provides roughly 20% smaller patches, is described in my doctoral thesis.
]]

local new_content = [[
    bsdiff and bspatch are tools for building and applying patches to binary files. By using suffix sorting (specifically, Larsson and Sadakane's qsufsort) and taking advantage of how executable files change, bsdiff routinely produces binary patches 50-80% smaller than those produced by Xdelta, and 15% smaller than those produced by .RTPatch (a $2750/seat commercial patch tool).
    These programs were originally named bdiff and bpatch, but the large number of other programs using those names lead to confusion; I'm not sure if the "bs" in refers to "binary software" (because bsdiff produces exceptionally small patches for executable files) or "bytewise subtraction" (which is the key to how well it performs). Feel free to offer other suggestions.
    
    bsdiff and bspatch use bzip2; by default they assume it is in /usr/bin.
    
    bsdiff is quite memory-hungry. It requires max(17*n,9*n+m)+O(1) bytes of memory, where n is the size of the old file and m is the size of the new file. bspatch requires n+m+O(1) bytes.
    
    HAHA, bsdiff runs in O((n+m) log n) time; on a 200MHz Pentium Pro, building a binary patch for a 4MB file takes about 90 seconds. bspatch runs in O(n+m) time; on the same machine, applying that patch takes about two seconds.

    Providing that off_t is defined properly, bsdiff and bspatch support files of up to 2^61-1 = 2Ei-1 bytes.

    Version 4.3 available here with MD5 hash e6d812394f0e0ecc8d5df255aa1db22a. Version 4.2 is available in the FreeBSD, NetBSD, and OpenBSD ports trees as misc/bsdiff, in Darwinports as devel/bsdiff, and in gentoo as dev-util/bsdiff. It has also been made into a Python extension module.

    The algorithm used by BSDiff 4 is described in my (unpublished) paper Naive differences of executable code; please cite this in papers as

    Colin Percival, Naive differences of executable code, http://www.daemonology.net/bsdiff/, 2003.
    A far more sophisticated algorithm, which typically provides roughly 20% smaller patches, is described in my doctoral thesis.
]]

-- generate patch
local patch, emsg = bsdiff.diff(old_content, new_content)
if emsg then
    print("diff error: ", emsg)
    os.exit(0)
end
print("patch length: ", patch:len())

-- generate another new_content
local out_content, emsg = bsdiff.patch(old_content, patch)
if emsg then
    print("patch error: ", emsg)
    os.exit(0)
end

print("content equal: ", new_content == out_content)