CXXFLAGS = -ffast-math -std=c++14 -Iinclude -DCHAISCRIPT_NO_THREADS -Wno-invalid-offsetof -O3 -fdiagnostics-color=always -fmax-errors=1
LDFLAGS = -L./lib -lminecraftpe

LPLAYER = -Lout -lsupport
LCHAI = out/ChaiSupport.so

MODS = chat command form base test tick

$(shell mkdir -p objs deps out >/dev/null)

.PHONY: all
all: out/libsupport.so out/ChaiSupport.so $(addsuffix .so,$(addprefix out/chai_,$(MODS)))

.PHONY: clean
clean:
	@rm -rf objs out deps

out/libsupport.so: objs/fix.o objs/string.o objs/support.o lib/libminecraftpe.so
	@echo LD $@
	@$(CPP) $(LDFLAGS) -shared -fPIC -o $@ $(filter %.o,$^)
out/chai_%.so: objs/%.o objs/hack.o out/libsupport.so lib/libminecraftpe.so out/ChaiSupport.so
	@echo LD $@
	@$(CPP) $(LDFLAGS) $(LPLAYER) $(LCHAI) -shared -fPIC -o $@ $(filter %.o,$^)
out/ChaiSupport.so: objs/fix.o objs/main.o lib/libminecraftpe.so out/libsupport.so
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