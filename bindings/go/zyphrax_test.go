package zyphrax

import (
	"bytes"
	"testing"
)

func TestCompress(t *testing.T) {
	data := []byte("Hello Go World")
	comp, err := Compress(data)
	if err != nil {
		t.Fatalf("Compression error: %v", err)
	}
	if len(comp) < 12 {
		t.Errorf("Result too short: %d", len(comp))
	}
}

func TestCompressLarge(t *testing.T) {
	data := bytes.Repeat([]byte("A"), 100000)
	comp, err := Compress(data)
	if err != nil {
		t.Fatal(err)
	}
	if len(comp) > 2000 {
		t.Errorf("Compression poor: %d bytes", len(comp))
	}
}
