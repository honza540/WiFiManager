#ifndef I_COMMAND_HANDLER_H
#define I_COMMAND_HANDLER_H

#include <Arduino.h>

/**
 * Interface for BT command handlers
 * Allows different modules to register their own commands
 */
class ICommandHandler {
public:
    virtual ~ICommandHandler() {}

    /**
     * Handle command execution
     * @return true if command was handled, false otherwise
     */
    virtual bool handleCommand(const String& cmd, const String& args) = 0;

    /**
     * Get help text for this handler's commands
     */
    virtual String getHelpText() = 0;
};

#endif // I_COMMAND_HANDLER_H
