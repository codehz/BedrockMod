CFLAGSQL = -std=c99 -Iinclude -O3 -fdiagnostics-color=always -fmax-errors=1
CXXFLAGS = -ffast-math -std=c++14 -Iinclude -DCHAISCRIPT_NO_THREADS -Wno-invalid-offsetof -O3 -fdiagnostics-color=always -fmax-errors=1
LDFLAGS = -L./lib -lminecraftpe

LPLAYER = -Lout -lsupport
LCHAI = -Lout -lchai

MODS = chat command form base test tick prop sqlite3 dbus_chat transfer

$(shell mkdir -p objs deps out ref >/dev/null)

.PHONY: all
all: out/libsupport.so out/libchai.so $(addsuffix .so,$(addprefix out/chai_,$(MODS)))

.PHONY: clean
clean:
	@rm -rf objs out deps ref

objs/sqlite.o: src/sqlite3.c
	@echo CC $@
	@$(CC) $(CFLAGSQL) -c -o $@ $<

ref/bridge.so: objs/sqlite.o
	@echo MO $@
	@$(CPP) -shared -fPIC -o $@ $(filter %.o,$^)

out/libsupport.so: objs/fix.o objs/string.o objs/support.o lib/libminecraftpe.so
	@echo LD $@
	@$(CPP) $(LDFLAGS) -shared -fPIC -o $@ $(filter %.o,$^)
out/chai_sqlite3.so: ref/bridge.so
out/chai_%.so: objs/%.o objs/hack.o out/libsupport.so lib/libminecraftpe.so out/libchai.so
	@echo LD $@
	@$(CPP) $(LDFLAGS) $(LPLAYER) $(LCHAI) -shared -fPIC -o $@ $(filter %.o,$^) $(addprefix -Lref -l:,$(notdir $(filter ref/%.so,$^)))
out/libchai.so: objs/fix.o objs/main.o lib/libminecraftpe.so out/libsupport.so
	@echo LD $@
	@$(CPP) $(LDFLAGS) $(LPLAYER) -shared -fPIC -o $@ $(filter %.o,$^)

.PRECIOUS: deps/%.d
deps/%.d: src/%.cpp
	@echo DP $<
	@$(CPP) $(CXXFLAGS) -MT $(patsubst deps/%.d,objs/%.o,$@) -M -o $@ $<

.PRECIOUS: objs/%.o
objs/%.o: src/%.cpp
objs/%.o: src/%.cpp deps/%.d
	@echo CC $@
	@$(CPP) $(CXXFLAGS) -c -o $@ $<

-include deps/*