const std = @import("std");

const mdgui_c_sources = &.{
    "src/mdgui_c.cpp",
    "mdgui_impl/glue/mdgui_glue.cpp",
    "mdgui_impl/backends/mdgui_backend_sdl.cpp",
    "mdgui_impl/backends/mdgui_backend_opengl.cpp",
    "mdgui_impl/backends/mdgui_backend_vulkan.cpp",
};

fn addDemo(
    b: *std.Build,
    name: []const u8,
    root_source_path: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    sdl_dep: *std.Build.Dependency,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = name,
        .root_module = b.createModule(.{
            .root_source_file = b.path(root_source_path),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.root_module.addIncludePath(sdl_dep.path("include"));
    exe.root_module.addIncludePath(b.path("include"));
    exe.linkLibrary(sdl_dep.artifact("SDL3"));
    exe.linkLibC();
    exe.linkLibCpp();

    exe.root_module.addCSourceFiles(.{
        .files = mdgui_c_sources,
        .flags = &.{ "-std=c++17", "-fpermissive" },
    });

    return exe;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });

    const sdl_demo = addDemo(
        b,
        "mdgui-demo",
        "demos/templates/sdl/main.zig",
        target,
        optimize,
        sdl_dep,
    );
    b.installArtifact(sdl_demo);

    const run_cmd = b.addRunArtifact(sdl_demo);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the SDL demo");
    run_step.dependOn(&run_cmd.step);

    const vulkan_demo = addDemo(
        b,
        "mdgui-demo-vulkan",
        "demos/templates/vulkan/main.zig",
        target,
        optimize,
        sdl_dep,
    );

    const build_vulkan_step = b.step("build-vulkan", "Build the Vulkan template demo");
    build_vulkan_step.dependOn(&vulkan_demo.step);

    const run_vulkan_cmd = b.addRunArtifact(vulkan_demo);
    const run_vulkan_step = b.step("run-vulkan", "Run the Vulkan template demo");
    run_vulkan_step.dependOn(&run_vulkan_cmd.step);
}
