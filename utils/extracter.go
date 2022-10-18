package utils

import "path"

func IsTar(filepath string) bool {
	switch path.Ext(filepath) {
	case ".xz", ".gz", ".tgz", ".txz", ".bz2", ".tbz", ".zip", ".zst", ".zstd":
		return true
	}
	return false
}
