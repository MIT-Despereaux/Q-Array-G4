from pathlib import Path
import subprocess
import argparse
import sys


def main(
        source_dir: str,
        dest_dir: str,
        user: str,
        ssh_key: str,
        rsync_args: str,
        num_proc: int,
        mkpath: bool,
        reverse: bool) -> int:
    # Main function
    # NOTE: pathlib will strip the trailing slash from the directory name.
    source_path = Path(source_dir).resolve()
    dest_dir = Path(dest_dir).resolve()
    dest_path = f"{user}@txe1-login.mit.edu:{dest_dir}"
    if reverse:
        source_path, dest_path = dest_path, source_path
        num_proc = 1
    print(f"Source: {source_path}")
    print(f"Destination: {dest_path}")
    if num_proc > 1:
        ls_cmd = ["ls", f"{source_path}"]
        ls_proc = subprocess.Popen(ls_cmd, stdout=subprocess.PIPE)
        rsync_cmd = [
            "xargs",
            "-n1",
            f"-P{num_proc}",
            "-I%",
            "rsync"]
        rsync_cmd.extend(rsync_args.split())
        if mkpath:
            rsync_cmd.extend(["--mkpath"])
        if len(ssh_key) != 0:
            print(f"Using ssh key: {ssh_key}.")
            ssh_args = ['-e', f'ssh -i {ssh_key}']
            rsync_cmd.extend(ssh_args)
        rsync_cmd.extend(
            [f"{source_path}/%",
             f"{dest_path}"])
        proc = subprocess.Popen(
            rsync_cmd,
            stdin=ls_proc.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
    else:
        rsync_cmd = ["rsync"]
        rsync_cmd.extend(rsync_args.split())
        if mkpath:
            rsync_cmd.extend(["--mkpath"])
        if len(ssh_key) != 0:
            print(f"Using ssh key: {ssh_key}.")
            ssh_args = ['-e', f'ssh -i {ssh_key}']
            rsync_cmd.extend(ssh_args)
        rsync_cmd.extend(
            [f"{source_path}/",
             f"{dest_path}"])
        proc = subprocess.Popen(
            rsync_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
    print(f"rsync command: {rsync_cmd}")

    while True:
        output = proc.stdout.readline()
        if proc.poll() is not None:
            break
        if output:
            print(output.strip().decode('ascii'))
    return proc.poll()


if __name__ == "__main__":
    # Parsing arguments
    PARSER = argparse.ArgumentParser(
        description="This script synchronizes local and MIT supercloud msmts.")

    PARSER.add_argument('source_dir',
                        metavar='SRC_DIR',
                        help='Speficies the source directory.',
                        type=str)
    PARSER.add_argument(
        'dest_dir',
        metavar='DEST_DIR',
        help='Speficies the destination directory. This directory MUST exist.',
        type=str)
    PARSER.add_argument('user',
                        metavar='USER',
                        help='Speficies the username on MIT supercloud.',
                        type=str)
    PARSER.add_argument(
        '-A',
        '--args',
        dest='rsync_args',
        help='Specifies args for rsync, separated by spaces. See the man page for details. Default: "-Prlhv"',
        default="-Prlhv",
        type=str)
    PARSER.add_argument(
        '-k',
        '--sshkey',
        dest='ssh_key',
        help='Specifies ssh key path (e.g. $HOME/.ssh/id_rsa). Defaults to empty.',
        default="",
        type=str)
    PARSER.add_argument(
        '-p', '--proc',
        dest='num_proc',
        help='Specifies the number of parallel processes.',
        default=1,
        type=int)
    PARSER.add_argument(
        '--mkpath',
        dest='mkpath',
        help='Specifies whether to make folders at the destination on demand.',
        action='store_true')
    PARSER.add_argument(
        '-R',
        '--reverse',
        dest='reverse',
        help='Reverses the direction. Does not support parallel downloading. The number of processes will be ignored.',
        action='store_true')

    ARGS: argparse.Namespace = PARSER.parse_args()

    sys.exit(main(
        ARGS.source_dir,
        ARGS.dest_dir,
        ARGS.user,
        ARGS.ssh_key,
        ARGS.rsync_args,
        ARGS.num_proc,
        ARGS.mkpath,
        ARGS.reverse))
