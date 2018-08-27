CFLAGSQL = -std=c99 -Iinclude -O3 -fdiagnostics-color=always -fmax-errors=1
CXXFLAGS = -ffast-math -std=c++14 -Iinclude -DCHAISCRIPT_NO_THREADS -Wno-invalid-offsetof -O3 -fdiagnostics-color=always -fmax-errors=1
LDFLAGS = -L./lib -lminecraftpe

LPLAYER = -Lout -lsupport
LCHAI = -Lout -lchai

MODS = $(shell ls src/mods)
CHAI_MODS = $(shell ls src/chai)

$(shell mkdir -p obj dep out ref >/dev/null)

.PHONY: all
all: $(addsuffix .so,$(addprefix out/lib,$(MODS)))  $(addsuffix .so,$(addprefix out/chai_,$(CHAI_MODS)))

.PHONY: clean
clean:
	@rm -rf obj out dep ref

obj/sqlite.o: src/sqlite3.c
	@echo CC $@
	@$(CC) $(CFLAGSQL) -c -o $@ $<

ref/bridge.so: obj/sqlite.o
	@echo MO $@
	@$(CXX) -shared -fPIC -o $@ $(filter %.o,$^)

out/libsupport.so: obj/fix.o obj/string.o obj/mods/support/main.o lib/libminecraftpe.so
	@echo LD $@
	@$(CXX) $(LDFLAGS) -shared -fPIC -o $@ $(filter %.o,$^)
out/chai_sqlite3.so: ref/bridge.so
out/chai_%.so: obj/chai/%/main.o obj/hack.o out/libsupport.so lib/libminecraftpe.so out/libchai.so
	@echo LD $@
	@$(CXX) $(LDFLAGS) $(LPLAYER) $(LCHAI) -shared -fPIC -o $@ $(filter %.o,$^) $(addprefix -Lref -l:,$(notdir $(filter ref/%.so,$^)))
out/libchai.so: obj/fix.o obj/mods/chai/main.o lib/libminecraftpe.so out/libsupport.so
	@echo LD $@
	@$(CXX) $(LDFLAGS) $(LPLAYER) -shared -fPIC -o $@ $(filter %.o,$^)

.PRECIOUS: dep/%.d
dep/%.d: src/%.cpp
	@echo DP $<
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -MT $(patsubst dep/%.d,obj/%.o,$@) -M -o $@ $<

.PRECIOUS: obj/%.o
obj/%.o: src/%.cpp
obj/%.o: src/%.cpp dep/%.d
	@echo CC $@
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

SOURCES = $(shell find src -name '*.cpp')

include $(patsubst src/%.cpp, dep/%.d, $(SOURCES))