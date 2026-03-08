const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const sdl_dep = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "mgui-demo",
        .root_module = b.createModule(.{
            .root_source_file = b.path("demo/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.root_module.addIncludePath(sdl_dep.path("include"));
    exe.linkLibrary(sdl_dep.artifact("SDL3"));
    exe.linkLibC();
    exe.linkLibCpp();

    exe.root_module.addIncludePath(b.path("include"));
    exe.root_module.addCSourceFiles(.{
        .files = &.{
            "src/mgui_c.cpp",
            "src/mgui_glue.cpp",
            "src/mgui_backend_sdl.cpp",
            "src/mgui_backend_opengl.cpp",
            "src/mgui_backend_vulkan.cpp",
        },
        .flags = &.{ "-std=c++17", "-fpermissive" },
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the demo");
    run_step.dependOn(&run_cmd.step);
}
