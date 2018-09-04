CFLAGSQL = -std=c99 -Iinclude -O3 -fdiagnostics-color=always -fmax-errors=1
CXXFLAGS = -ffast-math -std=c++14 -Iinclude -Iinclude/guile/2.2 -Iinclude/gmp -Wno-invalid-offsetof -O3 -fdiagnostics-color=always -fmax-errors=1
LDFLAGS = -L./lib -lminecraftpe

LPLAYER = -Lout -lsupport
LSCRIPT = -Lout -lscript

MODS = $(shell ls src/mods)
# SCRIPT_MODS = $(shell ls src/script)
SCRIPT_MODS = base chat tick

$(shell mkdir -p obj dep out ref >/dev/null)

.PHONY: all
all: $(addsuffix .so,$(addprefix out/lib,$(MODS)))  $(addsuffix .so,$(addprefix out/script_,$(SCRIPT_MODS)))

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
out/libscript.so: obj/fix.o obj/mods/script/main.o lib/libminecraftpe.so out/libsupport.so
	@echo LD $@
	@$(CXX) $(LDFLAGS) $(LPLAYER) -shared -fPIC -o $@ $(filter %.o,$^)
out/script_sqlite3.so: ref/bridge.so
out/script_%.so: obj/script/%/main.o obj/hack.o out/libsupport.so lib/libminecraftpe.so out/libscript.so
	@echo LD $@ $^
	$(CXX) $(LDFLAGS) -shared -fPIC -o $@ $(filter %.o,$^) $(addprefix -Lref -l:,$(notdir $(filter ref/%.so,$^))) $(addprefix -Lout -l:,$(notdir $(filter out/%.so,$^)))

.PRECIOUS: dep/%.d
dep/%.d: src/%.cpp
	@echo DP $< 
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -MT $(patsubst dep/%.d,obj/%.o,$@) -M -MG -o $@ $<
	@sed -i 's/ \([^/]\+\).x/ '$(subst /,\\/,$(<D))'\/\1.x/' $@
	@-grep -oP '(?<=// Deps: ).+' $< >> $@

.PRECIOUS: .x
src/%.x: src/%.cpp
	@echo SN $@
	@guile-snarf -o $@ $< $(CXXFLAGS)

.PRECIOUS: obj/%.o
obj/%.o: src/%.cpp
obj/%.o: src/%.cpp dep/%.d
	@echo CC $@
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

SOURCES = $(shell find src -name '*.cpp')

include $(patsubst src/%.cpp, dep/%.d, $(SOURCES))