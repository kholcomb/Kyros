# Contributing to Kyros

Thank you for your interest in contributing to Kyros!

## Development Setup

1. Clone the repository
2. Install dependencies (see README.md)
3. Build the project: `./build.sh Debug`
4. Run tests: `cd build && ctest`

## Code Style

- Follow C++17 best practices
- Use meaningful variable names
- Comment complex logic
- Keep functions focused (single responsibility)
- Prefer RAII for resource management

## Pull Request Process

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## Testing

- All new features must include tests
- Maintain test coverage above 80%
- Test on multiple platforms when possible

## Documentation

- Update relevant documentation
- Add docstrings for new public APIs
- Update Kyros.md specification if needed

## Questions?

Open an issue for discussion before major changes.
