# See README.txt.

PBC_STAMP=.pbc-stamp

.PHONY: all

all:    add_person list_people dump_people c-dump

clean:
	@rm -f add_person list_people dump_people
	@rm -f $(PBC_STAMP) addressbook.pb.cc addressbook.pb.h

$(PBC_STAMP): addressbook.proto
	@protoc --cpp_out=. addressbook.proto
	@protoc-c --c_out=. addressbook.proto
	@touch $(PBC_STAMP)

add_person: add_person.cc $(PBC_STAMP)
	@c++ add_person.cc addressbook.pb.cc -o add_person `pkg-config --cflags --libs protobuf`
	@indent -kr -nut -i2 addressbook.pb-c.h
	@indent -kr -nut -i2 addressbook.pb-c.c
	@rm -f addressbook.pb-c.[hc]~

list_people: list_people.cc $(PBC_STAMP)
	@c++ list_people.cc addressbook.pb.cc -o list_people `pkg-config --cflags --libs protobuf`

dump_people: dump_people.cc $(PBC_STAMP)
	@c++ dump_people.cc addressbook.pb.cc -o dump_people `pkg-config --cflags --libs protobuf`

c-dump: c-dump.c addressbook.pb-c.c $(PBC_STAMP)
	cc -g -O0 -o c-dump c-dump.c addressbook.pb-c.c -lprotobuf-c

moo.ab:
	./add_person moo.ab

.PHONY: test
test: c-dump dump_people add_person moo.ab
	./c-dump moo.ab
	./dump_people moo.ab
