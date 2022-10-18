package utils

import (
	"encoding/json"
	"encoding/xml"
	"fmt"
	"io/ioutil"
	"path"
	"strings"

	"gopkg.in/yaml.v2"
)

type SupportedFormat string

const (
	YAML SupportedFormat = "yaml"
	YML  SupportedFormat = "yml"
	JSON SupportedFormat = "json"
	XML  SupportedFormat = "xml"
)

// FromFile load the data from file and format is taked from extension
// Supported extensions
//  - .xml
//  - .json
//  - .yaml .yml
func FromFile[T any](filepath string) (*T, error) {
	buffer, err := ioutil.ReadFile(filepath)
	if err != nil {
		return nil, fmt.Errorf("failed to read data file %s, %s", filepath, err)
	}

	ext := strings.TrimPrefix(path.Ext(filepath), ".")
	return FromBuffer[T](buffer, SupportedFormat(ext))
}

// FromBuffer load the  data from buffer with format specified
func FromBuffer[T any](buffer []byte, format SupportedFormat) (*T, error) {
	var data T
	var err error
	switch format {
	case YAML, YML:
		err = yaml.Unmarshal(buffer, &data)
	case JSON:
		err = json.Unmarshal(buffer, &data)
	case XML:
		err = xml.Unmarshal(buffer, &data)
	default:
		return nil, fmt.Errorf("unsupported format %s", format)
	}
	return &data, err
}
